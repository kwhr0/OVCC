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

#include "defines.h"
#include "tcc1014mmu.h"
#include "tcc1014registers.h"
#include "tcc1014graphicsAGAR.h"
#include "coco3.h"
#include "keyboard.h"
#include "vcc.h"


static unsigned char VDG_Mode=0;
static unsigned char Dis_Offset=0;
static unsigned char MPU_Rate=0;
static unsigned char *rom;
static unsigned char GimeRegisters[256];
static unsigned short VerticalOffsetRegister=0;
static unsigned char EnhancedFIRQFlag=0,EnhancedIRQFlag=0;
static int InterruptTimer=0;
void SetInit0(unsigned char);
void SetInit1(unsigned char);
void SetGimeIRQStearing(unsigned char);
void SetGimeFIRQStearing(unsigned char);
void SetTimerMSB(unsigned char);
void SetTimerLSB(unsigned char);
unsigned char GetInit0(unsigned char port);
static unsigned char IRQStearing[8]={0,0,0,0,0,0,0,0};
static unsigned char FIRQStearing[8]={0,0,0,0,0,0,0,0};
static unsigned char LastIrq=0,LastFirq=0,Temp=0;

static unsigned char KeyboardInterruptEnabled = 0;

unsigned char GimeGetKeyboardInterruptState()
{
	return KeyboardInterruptEnabled;
}

void GimeSetKeyboardInterruptState(unsigned char State)
{
	KeyboardInterruptEnabled = !!State;
}

void GimeWrite(unsigned char port,unsigned char data)
{
	GimeRegisters[port]=data;

	// if ((port == 0x92 || port == 0x93) && data & 32) write(0, "*", 1);

	switch (port)
	{
	case 0x90:
		SetInit0(data);
		break;

	case 0x91:
		SetInit1(data);
		break;

	case 0x92:
		SetGimeIRQStearing(data);
		break;

	case 0x93:
		SetGimeFIRQStearing(data);
		break;

	case 0x94:
		SetTimerMSB(data);
		break;

	case 0x95:
		SetTimerLSB(data);
		break;

	case 0x96:
		SetTurboMode(data & 1);
		break;
	case 0x97:
		break;

	case 0x98:
		SetGimeVmodeAGAR(data);
		break;

	case 0x99:
		SetGimeVresAGAR(data);
		break;

	case 0x9A:
		SetGimeBoarderColorAGAR(data);
		break;

	case 0x9B:
		SetDistoRamBank(data);
		break;

	case 0x9C:
		break;

	case 0x9D:
	case 0x9E:
		SetVerticalOffsetRegisterAGAR((GimeRegisters[0x9D]<<8) | GimeRegisters[0x9E]);
		break;

	case 0x9F:
		SetGimeHorzOffsetAGAR(data);
		break;

	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	case 0xA4:
	case 0xA5:
	case 0xA6:
	case 0xA7:
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAE:
	case 0xAF:
		SetMmuRegister(port,data);
		break;

	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7:
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBE:
	case 0xBF:
		SetGimePalletAGAR(port-0xB0,data & 63);
		break;
	}
	return;
}

unsigned char GimeRead(unsigned char port)
{
	switch (port)
	{
	case 0x92:
		Temp=LastIrq;
		LastIrq=0;
		return(Temp);
		break;

	case 0x93:
		Temp=LastFirq;
		LastFirq=0;
		return(Temp);
		break;

	case 0x94:
	case 0x95:
		return(126);
		break;

	default:
		return(GimeRegisters[port]);
	}
}

void SetInit0(unsigned char data)
{
	SetCompatModeAGAR ( !!(data & 128));
	Set_MmuEnabled (!!(data & 64)); //MMUEN
	SetRomMap ( data & 3);			//MC0-MC1
	SetVectors( data & 8);			//MC3
	EnhancedFIRQFlag=(data & 16)>>4;
	EnhancedIRQFlag=(data & 32)>>5;
	return;
}

void SetInit1(unsigned char data)
{
	Set_MmuTask(data & 1);			//TR
	SetTimerClockRate (data & 32);	//TINS
	return;
}

unsigned char GetInit0(unsigned char port)
{
	unsigned char data=0;
	return(data);
}

void SetGimeIRQStearing(unsigned char data) //92
{
	if ( (GimeRegisters[0x92] & 2) | (GimeRegisters[0x93] & 2) )
		GimeSetKeyboardInterruptState(1);
	else
		GimeSetKeyboardInterruptState(0);

	if ( (GimeRegisters[0x92] & 8) | (GimeRegisters[0x93] & 8) )
		SetVertInterruptState(1); 
	else
		SetVertInterruptState(0);

	if ( (GimeRegisters[0x92] & 16) | (GimeRegisters[0x93] & 16) )
		SetHorzInterruptState(1);
	else
		SetHorzInterruptState(0);

	if ( (GimeRegisters[0x92] & 32) | (GimeRegisters[0x93] & 32) )
		SetTimerInterruptState(1);
	else
		SetTimerInterruptState(0);
	return;
}

void SetGimeFIRQStearing(unsigned char data) //93
{
	if ( (GimeRegisters[0x92] & 2) | (GimeRegisters[0x93] & 2) )
		GimeSetKeyboardInterruptState(1);
	else
		GimeSetKeyboardInterruptState(0);

	if ( (GimeRegisters[0x92] & 8) | (GimeRegisters[0x93] & 8) )
		SetVertInterruptState(1);
	else
		SetVertInterruptState(0);

	if ( (GimeRegisters[0x92] & 16) | (GimeRegisters[0x93] & 16) )
		SetHorzInterruptState(1);
	else
		SetHorzInterruptState(0);
	// Moon Patrol Demo Using Timer for FIRQ Side Scroll 
	if ( (GimeRegisters[0x92] & 32) | (GimeRegisters[0x93] & 32) )
		SetTimerInterruptState(1);
	else
		SetTimerInterruptState(0);

	return;
}

void SetTimerMSB(unsigned char data) //94
{
	unsigned short Temp;
	Temp=((GimeRegisters[0x94] <<8)+ GimeRegisters[0x95]) & 4095;
	SetInterruptTimer(Temp);
	return;	
}

void SetTimerLSB(unsigned char data) //95
{
	// unsigned short Temp;
	// Temp=((GimeRegisters[0x94] <<8)+ GimeRegisters[0x95]) & 4095;
	// SetInterruptTimer(Temp);
	return;
}

void GimeAssertKeyboardInterrupt(void) 
{
	if ( ((GimeRegisters[0x93] & 2)!=0) & (EnhancedFIRQFlag==1))
	{
		CPUAssertInterrupt(FIRQ,0);
		LastFirq |= 2;
	}
	else
	if ( ((GimeRegisters[0x92] & 2)!=0) & (EnhancedIRQFlag==1))
	{
		CPUAssertInterrupt(IRQ,0);
		LastIrq |= 2;
	}
	return;
}

void GimeAssertVertInterrupt(void)
{

	if (((GimeRegisters[0x93] & 8)!=0) & (EnhancedFIRQFlag==1))
	{
		//write(0, "F", 1);
		CPUAssertInterrupt(FIRQ,0); //FIRQ
		LastFirq |= 8;
	}
	else
	if (((GimeRegisters[0x92] & 8)!=0) & (EnhancedIRQFlag==1))
	{
		//write(0, "I", 1);
		CPUAssertInterrupt(IRQ,0); //IRQ moon patrol demo using this
		LastIrq |= 8;
	}
	return;
}

void GimeAssertHorzInterrupt(void)
{

	if (((GimeRegisters[0x93] & 16)!=0) & (EnhancedFIRQFlag==1))
	{
		CPUAssertInterrupt(FIRQ,0);
		LastFirq |= 16;
	}
	else
	if (((GimeRegisters[0x92] & 16)!=0) & (EnhancedIRQFlag==1))
	{
		CPUAssertInterrupt(IRQ,0);
		LastIrq |= 16;
	}
	return;
}

void GimeAssertTimerInterrupt(void)
{
	if (((GimeRegisters[0x93] & 32)!=0) & (EnhancedFIRQFlag==1))
	{
		CPUAssertInterrupt(FIRQ,0);
		LastFirq |= 32;
	}
	else
		if (((GimeRegisters[0x92] & 32)!=0) & (EnhancedIRQFlag==1))
		{
			CPUAssertInterrupt(IRQ,0);
			LastIrq |= 32;
		}
	return;
}

unsigned char sam_read(unsigned char port) //SAM don't talk much :)
{
	
	if ( (port>=0xF0) & (port <=0xFF)) //IRQ vectors from rom
		return( rom[0x3F00 + port]);

	return(0);
}
void sam_write(unsigned char data ,unsigned char port)
{
	unsigned char mask=0;
	unsigned char reg=0;

	if ((port >=0xC6) & (port <=0xD3))	//VDG Display offset Section
	{
		port=port-0xC6;
		reg= ((port & 0x0E)>>1);
		mask= 1<<reg;
		Dis_Offset= Dis_Offset & (0xFF-mask); //Shut the bit off
		if (port & 1)
			Dis_Offset= Dis_Offset | mask;
		SetGimeVdgOffsetAGAR(Dis_Offset);
	}

	if ((port >=0xC0) & (port <=0xC5))	//VDG Mode
	{
		port=port-0xC0;
		reg= ((port & 0x0E)>>1);
		mask= 1<<reg;
		VDG_Mode = VDG_Mode & (0xFF-mask);
		if (port & 1)
			VDG_Mode = VDG_Mode | mask;
		SetGimeVdgModeAGAR(VDG_Mode);
	}

	if ( (port==0xDE) | (port ==0xDF))
		SetMapType(port&1);

	if ( (port==0xD7) | (port==0xD9) )
		SetCPUMultiplyerFlag (1);

	if ( (port==0xD6) | (port==0xD8) )
		SetCPUMultiplyerFlag (0);

	return;
}


void mc6883_reset()
{
	VDG_Mode=0;
	Dis_Offset=0;
	MPU_Rate=0;
	rom=Getint_rom_pointer();
	return;
}

unsigned char VDG_Offset(void)
{
	return(Dis_Offset);
}

unsigned char VDG_Modes(void)
{
	return(VDG_Mode);
}

