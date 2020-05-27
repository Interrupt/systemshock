/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.
Copyright (C) 2018-2020 Shockolate project

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

#include <SDL.h>
#include "shockolate_version.h"
#include "InitMac.h"
#include "OpenGL.h"
#include "Prefs.h"
#include "ShockBitmap.h"
#include "Shock.h"
#include "ShockHelp.h"

#include "amaploop.h"
#include "map.h"
#include "gr2ss.h"
#include "frflags.h"
#include "version.h"


SDL_AudioDeviceID device;
SDL_Color gamePalette[256];
SDL_Palette *sdlPalette;
SDL_Renderer *renderer;
SDL_Window *window;

bool UseCutscenePalette = FALSE; // see cutsloop.c
bool MouseCaptured = FALSE;
char window_title[128];

extern grs_screen *svga_screen;
extern int mlook_enabled;

void InitSDL() {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        DEBUG("%s: Init failed", __FUNCTION__);
    }

    // TODO: figure out some universal set of settings that work...
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);

    gr_init();

    extern short svga_mode_data[];
    gr_set_mode(svga_mode_data[gShockPrefs.doVideoMode], TRUE);

    INFO("Setting up screen and render contexts");

    // Create a canvas to draw to

    SetupOffscreenBitmaps(grd_cap->w, grd_cap->h);

    // Open our window!

    sprintf(window_title, "System Shock - %s", SHOCKOLATE_VERSION);

    window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, grd_cap->w, grd_cap->h,
                              SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);

    // Create the palette

    sdlPalette = SDL_AllocPalette(256);

    // Setup the screen

    svga_screen = cit_screen = gr_alloc_screen(grd_cap->w, grd_cap->h);
    gr_set_screen(svga_screen);

    gr_alloc_ipal();

    SDL_ShowCursor(SDL_DISABLE);

    atexit(SDL_Quit);

    SDL_RaiseWindow(window);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(renderer, grd_cap->w, grd_cap->h);

    // Startup OpenGL

    init_opengl();

    SDLDraw();

    SDL_ShowWindow(window);
}

void SetSDLPalette(int index, int count, uchar *pal) {
    static bool gammalut_init = 0;
    static uchar gammalut[100 - 10 + 1][256];
    if (!gammalut_init) {
        double factor = (can_use_opengl() ? 1.0 : 2.2); // OpenGL uses 2.2
        int i, j;
        for (i = 10; i <= 100; i++) {
            double gamma = (double)i * 1.0 / 100;
            gamma = 1 - gamma;
            gamma *= gamma;
            gamma = 1 - gamma;
            gamma = 1 / (gamma * factor);
            for (j = 0; j < 256; j++)
                gammalut[i - 10][j] = (uchar)(pow((double)j / 255, gamma) * 255);
        }
        gammalut_init = 1;
        INFO("Gamma LUT init\'ed");
    }

    int gam = gShockPrefs.doGamma;
    if (gam < 10)
        gam = 10;
    if (gam > 100)
        gam = 100;
    gam -= 10;

    for (int i = index; i < index + count; i++) {
        gamePalette[i].r = gammalut[gam][*pal++];
        gamePalette[i].g = gammalut[gam][*pal++];
        gamePalette[i].b = gammalut[gam][*pal++];
        gamePalette[i].a = 0xff;
    }

    if (!UseCutscenePalette) {
        // Hack black!
        gamePalette[255].r = 0x0;
        gamePalette[255].g = 0x0;
        gamePalette[255].b = 0x0;
        gamePalette[255].a = 0xff;
    }

    SDL_SetPaletteColors(sdlPalette, gamePalette, 0, 256);
    SDL_SetSurfacePalette(drawSurface, sdlPalette);
    SDL_SetSurfacePalette(offscreenDrawSurface, sdlPalette);

    if (should_opengl_swap())
        opengl_change_palette();
}

void SDLDraw() {
    if (should_opengl_swap()) {
        sdlPalette->colors[255].a = 0x00;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, drawSurface);

    if (should_opengl_swap()) {
        sdlPalette->colors[255].a = 0xff;
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    }

    SDL_Rect srcRect = {0, 0, gScreenWide, gScreenHigh};
    SDL_RenderCopy(renderer, texture, &srcRect, NULL);
    SDL_DestroyTexture(texture);

    if (should_opengl_swap()) {
        opengl_swap_and_restore();
    } else {
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
    }
}

void CaptureMouse(bool capture) {
    MouseCaptured = (capture && gShockPrefs.goCaptureMouse);

    if (!MouseCaptured && mlook_enabled && SDL_GetRelativeMouseMode() == SDL_TRUE) {
        SDL_SetRelativeMouseMode(SDL_FALSE);

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_WarpMouseInWindow(window, w / 2, h / 2);
    } else
        SDL_SetRelativeMouseMode(MouseCaptured ? SDL_TRUE : SDL_FALSE);
}
