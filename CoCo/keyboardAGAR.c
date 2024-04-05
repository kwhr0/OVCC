/*****************************************************************************/
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
/*****************************************************************************/
/*
	Keyboard handling / translation - system -> emulator

	TODO: move joystick code out of here
	TODO: any key(s) that results in a multi-key (eg. SHIFT/ALT/CTRL) 
	        combination on the CoCo side is only sending one interrupt 
			signal for both keys.  This seems to work in virtually all cases, 
			but is it an accurate emulation?
*/
/*****************************************************************************/

#define DIRECTINPUT_VERSION 0x0800
// this must be before defines.h as it contains Windows types and not Windows.h include
#include <agar/core.h>
#include <agar/gui.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "defines.h"
#include "keyboard.h"
#include "mc6821.h"
#include "tcc1014registers.h"
#include "vcc.h"
#include "joystickinputSDL.h"
#include "keyboardLayout.h"

#include "xdebug.h"

/*****************************************************************************/
/*
	Forward declarations
*/

char SetMouseStatusSDL(short, unsigned char);

/*****************************************************************************/
/*
	Global variables
*/

//
// Joystick
//

SDL_Joystick *Stick1 = NULL;

static unsigned short StickValue = 0;

JoyStick LeftSDL;
JoyStick RightSDL;

static unsigned short LeftStickX = 32;
static unsigned short LeftStickY = 32;
static unsigned short RightStickX = 32;
static unsigned short RightStickY = 32;
static unsigned char LeftButton1Status = 0;
static unsigned char RightButton1Status = 0;
static unsigned char LeftButton2Status = 0;
static unsigned char RightButton2Status = 0;
static unsigned char LeftStickNumber = 0;
static unsigned char RightStickNumber = 0;

unsigned char ComparatorSetByDischarge = 0;

/*****************************************************************************/
//
// keyboard
//

#define KBTABLE_ENTRY_COUNT 160	///< key translation table maximum size, (arbitrary) most of the layouts are < 80 entries
#define KEY_DOWN	1
#define KEY_UP		0

/** track all keyboard scan codes state (up/down) */
static int ScanTable[512];

/** run-time 'rollover' table to pass to the MC6821 when a key is pressed */
static unsigned char RolloverTable[8];	// CoCo 'keys' for emulator

/** run-time key translation table - convert key up/down messages to 'rollover' codes */
static keytranslationentry_t *KeyTransTable;	// run-time keyboard layout table (key(s) to keys(s) translation)

/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/**
	Get CoCo 'scan' code

	Only called from MC6821.c to read the keyboard/joystick state

	should be a push instead of a pull?
*/
unsigned char vccKeyboardGetScanAGAR(unsigned char Col)
{
	unsigned char temp;
	unsigned char x;
	unsigned char mask;
	unsigned char ret_val;

	ret_val = 0;

	temp = ~Col; //Get colums
	mask = 1;

	//fprintf(stderr, "?");
	for (x=0; x<8; x++)
	{
		if ( (temp & mask) ) // Found an active column scan
		{
			ret_val |= RolloverTable[x];
		}

		mask = (mask<<1);
	}
	ret_val = 127 - ret_val;

	switch (LeftSDL.HiRes)
	{
		case 0: // Regular joystick mouse
			//	MuxSelect=GetMuxState();	//Collect CA2 and CB2 from the PIA (1of4 Multiplexer)
			StickValue = get_pot_valueSDL(GetMuxState());
			if (StickValue != 0)		//OS9 joyin routine needs this (koronis rift works now)
			{
				if (StickValue >= DACState())		// Set bit of stick >= DAC output $FF20 Bits 7-2
				{
					ret_val |= 0x80;
				}
			}
			break;

		case 1: // Hi-res joystick mouse
			// if (ComparatorSetByDischarge) {
			// 	fprintf(stdout, "#"); fflush(stdout);
			// }

			ret_val |= ComparatorSetByDischarge;  // this gets set in the main CPU exec loop if a DAC discharge has occurred
			ComparatorSetByDischarge = 0;  // It appears reading the comparator bit clears the bit. So must do this!
			break;

		default:
			break;
	}

	if (LeftButton1Status == 1)
	{
		//Left Joystick Button 1 Down?
		//fprintf(stderr, "B1D ");
		ret_val = ret_val & 0xFD;
	}

	if (RightButton1Status == 1)
	{
		//Right Joystick Button 1 Down?
		ret_val = ret_val & 0xFE;	
	}

	if (LeftButton2Status == 1)
	{
		//Left Joystick Button 2 Down?
		ret_val = ret_val & 0xF7;	
	}

	if (RightButton2Status == 1)
	{
		//Right Joystick Button 2 Down?
		ret_val = ret_val & 0xFB;	
	}

	#if 0 // no noticible change when this is disabled
	// TODO: move to MC6821/GIME
	{
		/** another keyboard IRQ flag - this should really be in the GIME code*/
		static unsigned char IrqFlag = 0;
		if ((ret_val & 0x7F) != 0x7F)
		{
			if ((IrqFlag == 0) & GimeGetKeyboardInterruptState())
			{
				GimeAssertKeyboardInterrupt();
				IrqFlag = 1;
			}
		}
		else
		{
			IrqFlag = 0;
		}
	}
	#endif

	//if (ret_val != 0xFF) fprintf(stderr, "<%2x>", ret_val);
	return (ret_val);
}

/*****************************************************************************/
/**
*/
static void _vccKeyboardUpdateRolloverTable()
{
	int Index;
	for (Index = 0; Index < 8; Index++)
	{
		RolloverTable[Index] = 0;
	}

	// set rollover table based on ScanTable key status
	for (Index = 0; Index<KBTABLE_ENTRY_COUNT; Index++)
	{
		if (!KeyTransTable[Index].ScanCode) break;
		if (ScanTable[KeyTransTable[Index].ScanCode] == KEY_DOWN)
		{
			int col = KeyTransTable[Index].Col1;
			assert(col >=0 && col <8);
			RolloverTable[col] |= KeyTransTable[Index].Row1;
			col = KeyTransTable[Index].Col2;
			assert(col >= 0 && col <8);
			RolloverTable[col] |= KeyTransTable[Index].Row2;
		}
	}
}

/*****************************************************************************/
/**
	Dispatch keyboard event to the emulator.

	Called from system. eg. WndProc : WM_KEYDOWN/WM_SYSKEYDOWN/WM_SYSKEYUP/WM_KEYUP

	@param key Windows virtual key code (VK_XXXX - not used)
	@param ScanCode keyboard scan code (DIK_XXXX - DirectInput)
	@param keyState Key status - kEventKeyDown/kEventKeyUp
*/
void vccKeyboardHandleKeySDL(unsigned short key, unsigned short ScanCode, unsigned short keyState)
{
	switch (keyState)
	{
		default:
			// internal error
		break;

		// Key Down
		case kEventKeyDown:
			if (  (LeftSDL.UseMouse == JOYSTICK_KEYBOARD)
				| (RightSDL.UseMouse == JOYSTICK_KEYBOARD)
				)
			{
				ScanCode = SetMouseStatusSDL(ScanCode, 1);
			}
			ScanTable[ScanCode] = KEY_DOWN;
			_vccKeyboardUpdateRolloverTable();
			if ( GimeGetKeyboardInterruptState() )
			{
				GimeAssertKeyboardInterrupt();
			}
		break;

		// Key Up
		case kEventKeyUp:
			if (  (LeftSDL.UseMouse == JOYSTICK_KEYBOARD)
				| (RightSDL.UseMouse == JOYSTICK_KEYBOARD)
				)
			{
				ScanCode = SetMouseStatusSDL(ScanCode, 0);
			}
			ScanTable[ScanCode] = KEY_UP;
			_vccKeyboardUpdateRolloverTable();
		break;
	}
}


void vccKeyboardBuildRuntimeTableAGAR(keyboardlayout_e keyBoardLayout)
{
	KeyTransTable = keyTranslationsNaturalAGAR;
}

/*****************************************************************************/
/*****************************************************************************/
/*
	Joystick related code
*/

/*****************************************************************************/
/**
*/
void joystickSDL(unsigned short x,unsigned short y)
{
	switch (LeftSDL.HiRes)
	{
		case 0: // Regular joystick mouse
            x = 64 * (x - 6) / 640;
            y = 64 * (y - 40) / 400;
			if (x>63) x=63;
			if (y>63) y=63;
			break;

		case 1:  // Hi-res joystick mouse こちらだとgshellでカーソルが動かない
			y *= 1.3;
			if (x>639) x=639;
			if (y>639) y=639;
			break;

		default:
			break;
	}

	if (LeftSDL.UseMouse==JOYSTICK_MOUSE)
	{
		LeftStickX=x;
		LeftStickY=y;
	}
	if (RightSDL.UseMouse==JOYSTICK_MOUSE)
	{
		RightStickX=x;
		RightStickY=y;
	}

	return;
}

void DoKeyBoardEvent(unsigned short key, unsigned short scancode, unsigned short state)
{
	vccKeyboardHandleKeySDL(key, scancode, state);
}

void DoButton(int button, int state)
{
	switch (button)
	{
		case SDL_BUTTON_LEFT:
			SetButtonStatusSDL(0, state);
		break;

		case SDL_BUTTON_RIGHT:
			SetButtonStatusSDL(1, state);
		break;
	}
}

/*****************************************************************************/
/**
*/
void SetStickNumbersSDL(unsigned char Temp1,unsigned char Temp2)
{
	LeftStickNumber=Temp1;
	RightStickNumber=Temp2;
	return;
}

/*****************************************************************************/
/**
*/
unsigned short get_pot_valueSDL(unsigned char pot)
{
	extern int gWindow;
	
	if (LeftSDL.UseMouse == JOYSTICK_JOYSTICK)
	{
		JoyStickPollSDL(&Stick1, LeftStickNumber);
		LeftStickX = (unsigned short)(SDL_JoystickGetAxis(Stick1, 0)+32678)>>10;
		LeftStickY = (unsigned short)(SDL_JoystickGetAxis(Stick1, 1)+32678)>>10;
		LeftButton1Status = SDL_JoystickGetButton(Stick1, 0);
		LeftButton2Status = SDL_JoystickGetButton(Stick1, 1);
	}

	if (RightSDL.UseMouse == JOYSTICK_JOYSTICK)
	{
		JoyStickPollSDL(&Stick1, RightStickNumber);
		RightStickX= (unsigned short)(SDL_JoystickGetAxis(Stick1, 0)+32678)>>10;
		RightStickY= (unsigned short)(SDL_JoystickGetAxis(Stick1, 1)+32678)>>10;
		RightButton1Status = SDL_JoystickGetButton(Stick1, 0);
		RightButton2Status = SDL_JoystickGetButton(Stick1, 1);
	}

	switch (pot)
	{
		case 0:
			return(RightStickX);
			break;

		case 1:
			return(RightStickY);
			break;

		case 2:
			return(LeftStickX);
			break;

		case 3:
			return(LeftStickY);
			break;
	}

	return (0);
}

/*****************************************************************************/
/**
*/
char SetMouseStatusSDL(short ScanCode,unsigned char Phase)
{
	char ReturnValue=ScanCode;

	switch (Phase)
	{
	case 0:
		if (LeftSDL.UseMouse==JOYSTICK_KEYBOARD)
		{
			if (ScanCode==LeftSDL.Left)
			{
				LeftStickX=32;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Right)
			{
				LeftStickX=32;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Up)
			{
				LeftStickY=32;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Down)
			{
				LeftStickY=32;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Fire1)
			{
				LeftButton1Status=0;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Fire2)
			{
				LeftButton2Status=0;
				ReturnValue=0;
			}
		}

		if (RightSDL.UseMouse==JOYSTICK_KEYBOARD)
		{
			if (ScanCode==RightSDL.Left)
			{
				RightStickX=32;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Right)
			{
				RightStickX=32;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Up)
			{
				RightStickY=32;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Down)
			{
				RightStickY=32;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Fire1)
			{
				RightButton1Status=0;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Fire2)
			{
				RightButton2Status=0;
				ReturnValue=0;
			}
		}
	break;

	case 1:
		if (LeftSDL.UseMouse==JOYSTICK_KEYBOARD)
		{
			if (ScanCode==LeftSDL.Left)
			{
				LeftStickX=0;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Right)
			{
				LeftStickX=63;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Up)
			{
				LeftStickY=0;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Down)
			{
				LeftStickY=63;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Fire1)
			{
				LeftButton1Status=1;
				ReturnValue=0;
			}
			if (ScanCode==LeftSDL.Fire2)
			{
				LeftButton2Status=1;
				ReturnValue=0;
			}
		}

		if (RightSDL.UseMouse==JOYSTICK_KEYBOARD)
		{
			if (ScanCode==RightSDL.Left)
			{
				ReturnValue=0;
				RightStickX=0;
			}
			if (ScanCode==RightSDL.Right)
			{
				RightStickX=63;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Up)
			{
				RightStickY=0;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Down)
			{
				RightStickY=63;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Fire1)
			{
				RightButton1Status=1;
				ReturnValue=0;
			}
			if (ScanCode==RightSDL.Fire2)
			{
				RightButton2Status=1;
				ReturnValue=0;
			}
		}
	break;
	}

	return(ReturnValue);
}

/*****************************************************************************/
/**
*/
void SetButtonStatusSDL(unsigned char Side,unsigned char State) //Side=0 Left Button Side=1 Right Button State 1=Down
{
	unsigned char Btemp=0;
	Btemp= (Side<<1) | State;
	if (LeftSDL.UseMouse == JOYSTICK_MOUSE)
		switch (Btemp)
		{
		case 0:
			LeftButton1Status=0;
		break;

		case 1:
			LeftButton1Status=1;
		break;

		case 2:
			LeftButton2Status=0;
		break;

		case 3:
			LeftButton2Status=1;
		break;
		}

	if (RightSDL.UseMouse == JOYSTICK_MOUSE)
		switch (Btemp)
		{
		case 0:
			RightButton1Status=0;
		break;

		case 1:
			RightButton1Status=1;
		break;

		case 2:
			RightButton2Status=0;
		break;

		case 3:
			RightButton2Status=1;
		break;
		}
}

void GetLeftJoystickValues(int *joyx, int *joyy)
{
	*joyx = LeftStickX;
	*joyy = LeftStickY;
}

void GetRightJoystickValues(int *joyx, int *joyy)
{
	*joyx = RightStickX;
	*joyy = RightStickY;
}
/*****************************************************************************/


