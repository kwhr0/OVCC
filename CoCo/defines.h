#ifndef __DEFINES_H__
#define __DEFINES_H__

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

#include <agar/core.h>
#include <agar/gui.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>

typedef int BOOL; 
typedef unsigned char UINT8;
typedef unsigned long DWORD;
typedef const char *  LPCTSTR;

typedef struct
{
    UINT8 R, G, B;
} RGB24bit;

#define MAX_PATH 260
#define FALSE 0
#define TRUE 1

// Emulation States
#define TH_RUNNING	0
#define TH_REQWAIT	1
#define TH_WAITING	2

//Speed throttling
#define FRAMEINTERVAL 120	//Number of frames to sum the framecounter over
#define TARGETFRAMERATE 60	//Number of throttled Frames per second to render
#define SAMPLESPERFRAME 262

//CPU 
#define FRAMESPERSECORD (double)59.923	//The coco really runs at about 59.923 Frames per second
#define LINESPERSCREEN (double)262
#define NANOSECOND (double)1000000000
#define COLORBURST (double)3579545 

//Misc
#define MAX_LOADSTRING 100
#define QUERY 255
#define INDEXTIME ((LINESPERSCREEN * TARGETFRAMERATE)/5)

//Common CPU defs
#define IRQ		1
#define FIRQ	2
#define NMI		3

extern void (*CPUInit)(void);
extern int  (*CPUExec)( int);
extern void (*CPUReset)(void);
extern void (*CPUAssertInterrupt)(unsigned char,unsigned char);
extern void (*CPUDeAssertInterrupt)(unsigned char);
extern void (*CPUForcePC)(unsigned short);

extern void _MessageBox(const char *);
extern char *GlobalExecFolder;

#ifdef _DEBUG
# ifdef DARWIN
extern FILE *logg;
# endif
#endif

typedef struct _tagPOINT
{
    long x;
    long y;
} _POINT, *_PPOINT;

typedef struct 
{
    AG_Window       *agwin;
    AG_Pixmap       *fx;
    AG_Thread       emuThread;
    AG_Thread       cpuThread;
    Uint16          Rendering;
    Uint16          Resizing;
    void            *Pixels;
    unsigned char	*RamBuffer;
    unsigned short	*WRamBuffer;
    unsigned char	RamSize;
    double			CPUCurrentSpeed;
    unsigned short	DoubleSpeedMultiplyer;
    unsigned char	DoubleSpeedFlag;
    unsigned char	TurboSpeedFlag;
    unsigned char	CpuType;
    unsigned char	MmuType;
    unsigned char	MouseType;
    unsigned char	FrameSkip;
    unsigned char	BitDepth;
    unsigned char	*PTRsurface8;
    unsigned short	*PTRsurface16;
    RGB24bit        *PTRsurface24;
    unsigned int	*PTRsurface32;
    long			SurfacePitch;
    unsigned short	LineCounter;
    unsigned char	ScanLines;
    unsigned char	EmulationRunning;
    unsigned char	ResetPending;
    _POINT			WindowSize;
    unsigned char	FullScreen;
    char			StatusLine[256];
} SystemState2;

static char RateList[4][7]={"Mute","11025","22050","44100"};
static unsigned short iRateList[4]={0,11025,22050,44100};
#define TAPEAUDIORATE 44100

#define BASE_CLOCK	1

#endif
