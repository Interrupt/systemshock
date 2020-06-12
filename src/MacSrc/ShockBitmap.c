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

//--------------------
//  Includes
//--------------------
#include "InitMac.h"
#include "Shock.h"
#include "ShockBitmap.h"
#include "2d.h"

//--------------------
//  Globals
//--------------------
SDL_Surface *drawSurface;
SDL_Surface *offscreenDrawSurface;

void ChangeScreenSize(int width, int height) {
    if (gScreenWide == width && gScreenHigh == height)
        return;

    INFO("ChangeScreenSize");

    SDL_RenderClear(renderer);

    extern bool fullscreenActive;
    SDL_SetWindowFullscreen(window, fullscreenActive ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

    SDL_SetWindowSize(window, width, height);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDL_RenderSetLogicalSize(renderer, width, height);

    SetupOffscreenBitmaps(width, height);

    gScreenWide = width;
    gScreenHigh = height;
}

//------------------------------------------------------------------------------------
//		Setup the main offscreen bitmaps.
//------------------------------------------------------------------------------------
void SetupOffscreenBitmaps(int width, int height) {
    DEBUG("SetupOffscreenBitmaps %i %i", width, height);

    if (drawSurface != NULL) {
        SDL_FreeSurface(drawSurface);
    }
    if (offscreenDrawSurface != NULL) {
        SDL_FreeSurface(offscreenDrawSurface);
    }

    drawSurface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
    if (!drawSurface) {
        ERROR("SDL: Failed to create draw surface");
        return;
    }

    offscreenDrawSurface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
    if (!offscreenDrawSurface) {
        ERROR("SDL: Failed to create offscreen draw surface");
        return;
    }

    // Point the renderer at the screen bytes
    gScreenRowbytes = drawSurface->w;
    gScreenAddress = drawSurface->pixels;

    grd_mode_cap.vbase = gScreenAddress;
}
