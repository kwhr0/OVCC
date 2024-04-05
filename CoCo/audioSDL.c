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
#include "defines.h"
#include "vcc.h"
#include "config.h"
#include "coco3.h"
#include "audio.h"
#include "logger.h"
#include "Wrap.h"

#define MAXCARDS	12
//PlayBack
static SDL_AudioDeviceID audioDev = 0;
static SDL_AudioSpec audioSpec, actualAudioSpec;
//Record

static unsigned short BitRate=0;
static unsigned char InitPassed=0;
static int CardCount=0;
static unsigned short CurrentRate=0;
static unsigned char AudioPause=0;
static SndCardList *Cards=NULL;

static void audioCallback(void *userData, uint8_t *stream, int len) {
	PSGSamples((int16_t *)stream, len >> 2);
	int16_t lmin = INT16_MAX, lmax = INT16_MIN, rmin = INT16_MAX, rmax = INT16_MIN;
	for (int16_t *p = (int16_t *)stream, *lim = p + (len >> 1); p < lim; p += 2) {
		int16_t l = p[0], r = p[1];
		if (lmin > l) lmin = l;
		if (lmax < l) lmax = l;
		if (rmin > r) rmin = r;
		if (rmax < r) rmax = r;
	}
	UpdateSoundBar(UINT16_MAX * sqrtf((float)(lmax - lmin) / UINT16_MAX),
				   UINT16_MAX * sqrtf((float)(rmax - rmin) / UINT16_MAX));
}

int SoundInitSDL (int devID, unsigned short Rate)
{
	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		if (SDL_Init(SDL_INIT_AUDIO) != 0)
		{
			fprintf(stderr, "Couldn't initialise SDL_Audio\n");
			return 0;
		}
	}

	Rate=(Rate & 3);
	if (Rate != 0)	//Force 44100 or Mute
		Rate = 3;

	CurrentRate=Rate;

	if (InitPassed)
	{
		InitPassed=0;
		SDL_PauseAudioDevice(audioDev, 1);

		if (audioDev!=0)
		{
			SDL_CloseAudioDevice(audioDev);
			audioDev=0;
		}
	}

	BitRate=iRateList[Rate];

	if (Rate)
	{
		audioSpec.callback = audioCallback;
		audioSpec.channels = 2;
		audioSpec.freq = BitRate;
		audioSpec.format = AUDIO_S16LSB;
		audioSpec.samples = 256;

		audioDev = SDL_OpenAudioDevice(Cards[devID].CardName, 0, &audioSpec, &actualAudioSpec, 0);
		//audioDev = SDL_OpenAudioDevice(Cards[devID].CardName, 0, NULL, NULL, 0);

		if (audioDev == 0)
			return(1);

		SDL_PauseAudioDevice(audioDev, 0);

		InitPassed=1;
		AudioPause=0;
	}

	SetAudioRate (iRateList[Rate]);
	return(0);
}

void FlushAudioBufferSDL(unsigned int *Abuffer,unsigned short Length)
{
/*
	unsigned short LeftAverage=0,RightAverage=0,Index=0;
	unsigned char Flag=0;
	LeftAverage=Abuffer[0]>>16;
	RightAverage=Abuffer[0]& 0xFFFF;
	UpdateSoundBar(LeftAverage,RightAverage);

	if ((!InitPassed) | (AudioPause))
		return;

	SDL_QueueAudio(audioDev, (void*)Abuffer, (Uint32)Length);
	SDL_PauseAudioDevice(audioDev, 0);
	Uint32 as = SDL_GetQueuedAudioSize(audioDev);
*/
}

int GetSoundCardListSDL (SndCardList *List)
{
	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		if (SDL_Init(SDL_INIT_AUDIO) != 0)
		{
			fprintf(stderr, "Couldn't initialise SDL_Audio\n");
			return 0;
		}
	}

	int totAudioDevs = SDL_GetNumAudioDevices(0);
	CardCount=0;
	Cards=List;

	for(CardCount = 0 ; CardCount < totAudioDevs && CardCount < MAXCARDS ; CardCount++)
	{
		strcpy(Cards[CardCount].CardName, SDL_GetAudioDeviceName(CardCount, 0));
		Cards[CardCount].sdlID = CardCount;
	}
	return(CardCount);
}

int SoundDeInitSDL(void)
{
	if (InitPassed)
	{
		InitPassed=0;
		SDL_PauseAudioDevice(audioDev, 1);
		SDL_CloseAudioDevice(audioDev);
		audioDev = 0;
	}
	return(0);
}

unsigned short GetSoundStatusSDL(void)
{
	return(CurrentRate);
}

void ResetAudioSDL (void)
{
	SetAudioRate (iRateList[CurrentRate]);
	return;
}

unsigned char PauseAudioSDL(unsigned char Pause)
{
	AudioPause=Pause;
	if (InitPassed)
	{
		SDL_PauseAudioDevice(audioDev, (int)AudioPause);
	}
	return(AudioPause);
}
