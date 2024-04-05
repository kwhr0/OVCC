/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <math.h>
#include "defines.h"
#include "tcc1014graphicsAGAR.h"
#include "tcc1014registers.h"
#include "mc6821.h"
#include "pakinterface.h"
#include "audio.h"
#include "coco3.h"
#include "throttle.h"
#include "vcc.h"
#include "cassette.h"
#include "AGARInterface.h"
#include "Wrap.h"

static double SoundInterrupt=0;
static double NanosToSoundSample=0;//SoundInterrupt;
static double NanosPerLine = NANOSECOND / (TARGETFRAMERATE * LINESPERSCREEN);
static double CyclesPerLine = ((COLORBURST/4)*(TARGETFRAMERATE/FRAMESPERSECORD)) / (TARGETFRAMERATE * LINESPERSCREEN);
static double CycleDrift=0;
static unsigned int StateSwitch=0;
static unsigned short SoundRate=0;

#define AUDIO_BUF_LEN 16384
#define CAS_BUF_LEN 8192

static unsigned char HorzInterruptEnabled=0,VertInterruptEnabled=0;
static unsigned char TopBoarder=0,BottomBoarder=0;
static unsigned char LinesperScreen;
static unsigned char TimerInterruptEnabled=0;
static int MasterTimer=0; 
static unsigned short TimerClockRate=0;
static double MasterTickCounter=0,UnxlatedTickCounter=0,OldMaster=0;
static unsigned char BlinkPhase=1;
static unsigned int AudioBuffer[AUDIO_BUF_LEN];
static unsigned char CassBuffer[CAS_BUF_LEN];
static unsigned short AudioIndex=0;
static double NanosToInterrupt=0;
static int IntEnable=0;
static int SndEnable=1;
static int OverClock=1;
static unsigned char SoundOutputMode=0;	//Default to Speaker 1= Cassette
void AudioOut(void);
void CassOut(void);
void CassIn(void);
void (*AudioEvent)(void)=AudioOut;
void SetMasterTickCounter(void);
void CPUConfigSpeedInc(void);
void CPUConfigSpeedDec(void);

volatile int processing;
static int FrameCounter;
extern int gThrottle;

typedef void (*DrawCallback)(SystemState2 *);
static void DrawLine(SystemState2 *RFState2, DrawCallback callback, int n) {
	if (!FrameCounter)
		for (RFState2->LineCounter = 0; RFState2->LineCounter < n; RFState2->LineCounter++)
			callback(RFState2);
}

float RenderFrame(SystemState2 *RFState2, unsigned long DCnt) {
	if (gThrottle) {
		vsync_wait();
		static int inccount;
		if (processing) CPUConfigSpeedDec();
		else if (!(++inccount & 0xf)) CPUConfigSpeedInc();
	}
	while (processing) usleep(100);
	//
    SetBlinkStateAGAR(BlinkPhase);
	irq_fs(0); // FS low to High transition start of display Boink needs this
	render_signal(); // release the cpu
	FrameCounter = (FrameCounter + 1) % RFState2->FrameSkip;
	DrawLine(RFState2, DrawTopBoarderAGAR, TopBoarder - 4);
	DrawLine(RFState2, UpdateScreen, LinesperScreen);
	irq_fs(1); // End of active display FS goes High to Low
	if (VertInterruptEnabled)
		GimeAssertVertInterrupt();
	DrawLine(RFState2, DrawBottomBoarderAGAR, BottomBoarder + 2);
	if (!FrameCounter) {
		UpdateAGAR(RFState2);
		SetBoarderChangeAGAR(0);
	}
	switch (SoundOutputMode) {
	case 0:
		FlushAudioBufferSDL(AudioBuffer,AudioIndex << 2);
		break;
	case 1:
		FlushCassetteBuffer(CassBuffer,AudioIndex);
		break;
	case 2:
		LoadCassetteBuffer(CassBuffer);
		break;
	}
	AudioIndex = 0;
    int fps = CalculateFPS();
    return FrameCounter ? 0 : fps;
}

void *CPUloop(void *RFState2) {
	while (CPUExec == NULL) { AG_Delay(1); } // The Render loop resets the system and the CPU must wait for reset 1st time;
	while (((SystemState2 *)RFState2)->agwin->visible) {
		render_wait();
		processing = 1;
		SetNatEmuStat(1 + MPUIsNative());
		// total iterations = 13 blanking lines + 4 non boarder lines + TopBoarder-4 lines + Active display lines + Bottom boarder lines + 6 vertical retrace lines
		// or TopBoarder + LinesPerScreen + Bottom Boarder + 19
		int NanosThisLine = NanosPerLine;
		for (int linecount = TopBoarder + LinesperScreen + BottomBoarder + 19; linecount;) {
			int sw = 0;
			double duration = NanosThisLine;
			if (duration > NanosToInterrupt && IntEnable) {
				duration = NanosToInterrupt;
				sw = 1;
			}
			if (duration > NanosToSoundSample && SndEnable) {
				duration = NanosToSoundSample;
				sw = 2;
			}
			int n = CycleDrift + (duration * CyclesPerLine * OverClock / NanosPerLine);
			CycleDrift = n >= 1 ? CPUExec(n) : n;
			switch (sw) {
				default:
					linecount--;
					if (HorzInterruptEnabled)
						GimeAssertHorzInterrupt();
					irq_hs(ANY);
					PakTimer();
					NanosThisLine = NanosPerLine;
					NanosToInterrupt -= duration;
					NanosToSoundSample -= duration;
					break;
				case 1:
					GimeAssertTimerInterrupt();
					NanosThisLine -= duration;
					NanosToInterrupt = MasterTickCounter;
					NanosToSoundSample -= duration;
					break;
				case 2:
					//AudioEvent();
					NanosThisLine -= duration;
					NanosToInterrupt -= duration;
					NanosToSoundSample = SoundInterrupt;
					break;
			}
		}
		processing = 0;
	}
	return NULL;
}

void SetClockSpeed(unsigned short Cycles)
{
	OverClock=Cycles;
	return;
}

void SetHorzInterruptState(unsigned char State)
{
	HorzInterruptEnabled= !!State;
	return;
}

void SetVertInterruptState(unsigned char State)
{
	VertInterruptEnabled= !!State;
	return;
}

void SetLinesperScreen (unsigned char Lines)
{
	Lines = (Lines & 3);
	LinesperScreen=Lpf[Lines];
	TopBoarder=VcenterTable[Lines];
	BottomBoarder= 243 - (TopBoarder + LinesperScreen); //4 lines of top boarder are unrendered 244-4=240 rendered scanlines
	return;
}

void SelectCPUExec(SystemState2* EmuState)
{
	if(EmuState->CpuType == 1) // 6309
	{
		CPUExec = Execute;
	}
}

void SetTimerInterruptState(unsigned char State)
{
	// printf("%d", (int)State); fflush(stdout);
	TimerInterruptEnabled=State;
	return;
}

void SetInterruptTimer(unsigned short Timer)
{
	UnxlatedTickCounter=(Timer & 0xFFF);
	SetMasterTickCounter();
	return;
}

void SetTimerClockRate (unsigned char Tmp)	//1= 279.265nS (1/ColorBurst) 
{											//0= 63.695uS  (1/60*262)  1 scanline time
	TimerClockRate=!!Tmp;
	SetMasterTickCounter();
	return;
}

void SetMasterTickCounter(void)
{
	double Rate[2]={NANOSECOND/(TARGETFRAMERATE*LINESPERSCREEN),NANOSECOND/COLORBURST};

	if (UnxlatedTickCounter==0)
		MasterTickCounter=0;
	else
		MasterTickCounter=(UnxlatedTickCounter+2)* Rate[TimerClockRate];

	if (MasterTickCounter != OldMaster)  
	{
		OldMaster=MasterTickCounter;
		NanosToInterrupt=MasterTickCounter;
	}

	if (MasterTickCounter!=0)
		IntEnable=1;
	else
		IntEnable=0;

	return;
}

void MiscReset(void)
{
	HorzInterruptEnabled=0;
	VertInterruptEnabled=0;
	TimerInterruptEnabled=0;
	MasterTimer=0; 
	TimerClockRate=0;
	MasterTickCounter=0;
	UnxlatedTickCounter=0;
	OldMaster=0;
//*************************
	SoundInterrupt=0;
	NanosToSoundSample=SoundInterrupt;
	CycleDrift=0;
	IntEnable=0;
	AudioIndex=0;
	ResetAudioSDL();
	return;
}

unsigned short SetAudioRate (unsigned short Rate)
{

	SndEnable=1;
	SoundInterrupt=0;
	CycleDrift=0;
	AudioIndex=0;
	if (Rate != 0)	//Force Mute or 44100Hz
		Rate = 44100;

	if (Rate==0)
		SndEnable=0;
	else
	{
		SoundInterrupt=NANOSECOND/Rate;
		NanosToSoundSample=SoundInterrupt;
	}
	SoundRate=Rate;
	return(0);
}

void AudioOut(void)
{
	if (AudioIndex < AUDIO_BUF_LEN)
	{
		AudioBuffer[AudioIndex++]=GetPSGSample();//GetDACSample();
	}
	return;
}

void CassOut(void)
{
	if (AudioIndex < CAS_BUF_LEN)
	{
		CassBuffer[AudioIndex++]=GetCasSample();
	}
	return;
}

void CassIn(void)
{
	if (AudioIndex < AUDIO_BUF_LEN)
	{
		AudioBuffer[AudioIndex]=GetDACSample();
		SetCassetteSample(CassBuffer[AudioIndex++]);
	}
	return;
}

unsigned char SetSndOutMode(unsigned char Mode)  //0 = Speaker 1= Cassette Out 2=Cassette In
{
	static unsigned char LastMode=0;
	static unsigned short PrimarySoundRate; 	PrimarySoundRate=SoundRate;

	switch (Mode)
	{
	case 0:
		if (LastMode==1)	//Send the last bits to be encoded
			FlushCassetteBuffer(CassBuffer,AudioIndex);

		AudioEvent=AudioOut;
		SetAudioRate (PrimarySoundRate);
		break;

	case 1:
		AudioEvent=CassOut;
		PrimarySoundRate=SoundRate;
		SetAudioRate (TAPEAUDIORATE);
		break;

	case 2:
		AudioEvent=CassIn;
		PrimarySoundRate=SoundRate;;
		SetAudioRate (TAPEAUDIORATE);
		break;

	default:	//QUERY
		return(SoundOutputMode);
		break;
	}

	if (Mode != LastMode)
	{
		AudioIndex=0;	//Reset Buffer on true mode switch
		LastMode=Mode;
	}

	SoundOutputMode=Mode;
	return(SoundOutputMode);
}
