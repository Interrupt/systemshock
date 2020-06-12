/*

Copyright (C) 1994-1995 Looking Glass Technologies, Inc.
Copyright (C) 2015-2018 Night Dive Studios, LLC.
Copyright (C) 2018-2020 Shockolate Project

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
// ShockBitmap.c - Manages off-screen bitmaps and palettes.

#include <SDL.h>

// Globals

// Current draw surface
extern SDL_Surface *drawSurface;
// Offscreen draw surface
extern SDL_Surface *offscreenDrawSurface;

// Prototypes

/// Change screen size.
void ChangeScreenSize(int width, int height);

/// Setup the main offscreen bitmaps.
void SetupOffscreenBitmaps(int width, int height);
