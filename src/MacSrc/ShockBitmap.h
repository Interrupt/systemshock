/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
//====================================================================================
//
//		System Shock - ©1994-1995 Looking Glass Technologies, Inc.
//
//		ShockBitmap.c	-	Manages off-screen bitmaps and palettes.
//
//====================================================================================

#include <SDL.h>

typedef struct ShockBitmap ShockBitmap; // FIXME: if this is actually needed, provide a portable struct

extern SDL_Surface *drawSurface;
extern SDL_Surface *offscreenDrawSurface;

//--------------------
//  Prototypes
//--------------------
void SetupOffscreenBitmaps(int width, int height);
