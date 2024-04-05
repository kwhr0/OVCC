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

#include <SDL2/SDL.h>
#include "throttle.h"
#include "defines.h"

static unsigned long Now;
static float cfps=0;

float CalculateFPS(void) //Done at end of render;
{

	static unsigned short FrameCount=0;
	static float fps=0,fNow=0,fLast=0;

	if (++FrameCount!=FRAMEINTERVAL)
		return(fps);

	Now = SDL_GetPerformanceCounter();
	fNow=(float)Now;
	fps=(fNow-fLast)/SDL_GetPerformanceFrequency();
	fps= FRAMEINTERVAL/fps;
	fLast=fNow;
	FrameCount=0;
	cfps = fps;
	return(fps);
}

float GetCurrentFPS()
{
	return cfps;
}

