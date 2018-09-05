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
//		Shock.c	-	Mac-specific initialization and main event loop.
//
//====================================================================================


//--------------------
//  Includes
//--------------------
//#include <Balloons.h>

#include <Carbon/Carbon.h>

#include "Shock.h"
#include "InitMac.h"
#include "OpenGL.h"
#include "Prefs.h"
#include "ShockBitmap.h"
#include "ShockHelp.h"
#ifdef TESTING
#include "Tests.h"
#endif

#include "amaploop.h"
#include "hkeyfunc.h"
#include "input.h"
#include "mainloop.h"
#include "setup.h"
#include "setploop.h"
#include "game_screen.h"
#include "fullscrn.h"
#include "status.h"
#include "gamewrap.h"
#include "faketime.h"
#include "map.h"
#include "frtypes.h"
#include "frprotox.h"
#include "gr2ss.h"
#include "frflags.h"
#include "player.h"
#include "physics.h"
#include "wrapper.h"
#include "version.h"

#include "Modding.h"

#include "cutsloop.h"

#include <stdint.h>
#include <SDL.h>

extern uchar game_paused;		// I've learned such bad lessons from LG.
extern uchar objdata_loaded;
extern uchar music_on;
extern uchar startup_music;

//--------------------
//  Globals
//--------------------
WindowPtr			gMainWindow;
MenuHandle		gMainMenus[kNumMenus];
//RgnHandle			gCursorRgn;
short				gCursorSet;
bool				gDone = false;
bool				gInForeground = true;
Boolean				gPlayingGame;		//¥¥¥ Temp
bool				gIsNewGame;
FSSpec				gSavedGameFile;
long					gGameSavedTime;
Boolean				gDeadPlayerQuit;
Boolean				gGameCompletedQuit;

grs_screen  *cit_screen;
SDL_Window* window;
SDL_Palette* sdlPalette;
SDL_Renderer* renderer;

char window_title[128];
int num_args;
char** arg_values;

extern grs_screen *svga_screen;
extern 	frc *svga_render_context;

//--------------------
//  Prototypes
//--------------------
extern void init_all(void);
extern void inv_change_fullscreen(uchar on);
extern void object_data_flush(void);
//extern Boolean IsFullscreenWareOn(void);
extern errtype load_da_palette(void);
extern void ShockMain(void);

void InitSDL();
void SDLDraw(void);
errtype CheckFreeSpace(short	checkRefNum);


//------------------------------------------------------------------------------------
//		Main function.
//------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	// Save the arguments for later

	num_args = argc;
	arg_values = argv;

	// FIXME externalize this
	log_set_quiet(0);
	log_set_level(LOG_INFO);

	// init mac managers

	InitMac();

	// Initialize the preferences file.

	SetDefaultPrefs();
	LoadPrefs();
	
#ifdef TESTING
	SetupTests();
#endif

	// Process some startup arguments

	bool show_splash = !CheckArgument("-nosplash");

	// CC: Modding support! This is so exciting.

	ProcessModArgs(argc, argv);

	// Initialize

	init_all();
	setup_init();

	gPlayingGame = TRUE;
	gDeadPlayerQuit = FALSE;
	gGameCompletedQuit = FALSE;

	load_da_palette();
	gr_clear(0xFF);

	// Draw the splash screen

	INFO("Showing splash screen");
	splash_draw(show_splash);

	// Start in the Main Menu loop

	_new_mode = _current_loop = SETUP_LOOP;
	loopmode_enter(SETUP_LOOP);

	// Start the main loop

	INFO("Showing main menu, starting game loop");
	mainloop(argc, argv);

	status_bio_end();
	stop_music();

	return 0;
}

bool CheckArgument(char* arg) {
	if(arg == NULL)
		return false;

	for(int i = 1; i < num_args; i++) {
		if(strcmp(arg_values[i], arg) == 0) {
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------------
//		Handle Quit menu command/apple event.
//------------------------------------------------------------------------------------
void DoQuit(void)
{
	gDone = true;
}

#define NEEDED_DISKSPACE   700000
//------------------------------------------------------------------------------------
//  See if we have enough free space to save the file.
//------------------------------------------------------------------------------------
errtype CheckFreeSpace(short	checkRefNum)
{
	// FIXME: This should probably do something?
	return (OK);
}

void InitSDL()
{
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		DebugString("SDL: Init failed");
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

	SetupOffscreenBitmaps();

	// Point the renderer at the screen bytes
	gScreenRowbytes = drawSurface->w;
	gScreenAddress = drawSurface->pixels;
	gScreenWide = 320;
	gScreenHigh = 200;
	gActiveLeft = 0;
	gActiveTop = 0;
	gActiveWide = 320;
	gActiveHigh = 200;

	gr_init();

	gr_set_mode(GRM_320x200x8, TRUE);

	INFO("Setting up screen and render contexts");

	// Open our window!

	sprintf(window_title, "System Shock - %s", SHOCKOLATE_VERSION);

	window = SDL_CreateWindow(
		window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		grd_cap->w, grd_cap->h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);


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
	SDL_RenderSetLogicalSize(renderer, gScreenWide, gScreenHigh);

	// Startup OpenGL

	init_opengl();

	SDLDraw();
}

SDL_Color gamePalette[256];
void SetSDLPalette(int index, int count, uchar *pal)
{
	for(int i = index; i < count; i++) {
		gamePalette[index+i].r = *pal++;
		gamePalette[index+i].g = *pal++;
		gamePalette[index+i].b = *pal++;
		gamePalette[index+i].a = 0xFF;
	}

	// Hack black!
	gamePalette[255].r = 0x0;
	gamePalette[255].g = 0x0;
	gamePalette[255].b = 0x0;
	gamePalette[255].a = 0xff;

	SDL_SetPaletteColors(sdlPalette, gamePalette, 0, count);
	SDL_SetSurfacePalette(drawSurface, sdlPalette);
	SDL_SetSurfacePalette(offscreenDrawSurface, sdlPalette);

	if(should_opengl_swap())
		opengl_change_palette();
}

void SDLDraw()
{
	sdlPalette->colors[255].a = 0x00;
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, drawSurface);
	sdlPalette->colors[255].a = 0xff;

	if (should_opengl_swap()) {
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	}

	SDL_Rect srcRect = { 0, 0, gScreenWide, gScreenHigh };
	SDL_RenderCopy(renderer, texture, &srcRect, NULL);
	SDL_DestroyTexture(texture);

	if (should_opengl_swap()) {
		opengl_swap_and_restore();
	} else {
		SDL_RenderPresent(renderer);
		SDL_RenderClear(renderer);
	}
}

bool MouseCaptured = FALSE;

extern int mlook_enabled;

void CaptureMouse(bool capture)
{
	MouseCaptured = (capture && gShockPrefs.goCaptureMouse);

	if (!MouseCaptured && mlook_enabled && SDL_GetRelativeMouseMode() == SDL_TRUE)
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);

		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		SDL_WarpMouseInWindow(window, w/2, h/2);
	}
	else SDL_SetRelativeMouseMode(MouseCaptured ? SDL_TRUE : SDL_FALSE);
}
