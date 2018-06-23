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

#include "Modding.h"

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

void ShockGameLoop(void);
void ShockSetupLoop(void);
void InitSDL();
void SDLDraw(void);
errtype CheckFreeSpace(short	checkRefNum);


//------------------------------------------------------------------------------------
//		Main function.
//------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	// FIXME externalize this
	log_set_quiet(0);
	log_set_level(LOG_TRACE);

	InitMac();															// init mac managers

	SetDefaultPrefs();													// Initialize the preferences file.
	//LoadPrefs(kPrefsResID);
	
#ifdef TESTING
	SetupTests();
#endif

	// CC: Modding support! This is so exciting.

	ProcessModArgs(argc, argv);

	// Initialize

	init_all();
	
	/*if (gShockPrefs.prefPlayIntro)
	{
		extern void PlayIntroCutScene(void);
		PlayIntroCutScene();
		gShockPrefs.prefPlayIntro = 0;
		SavePrefs(kPrefsResID);
	}*/

	extern errtype load_savegame_names(void);
	load_savegame_names();
	
	printf("Showing title screen\n");

	ShockSetupLoop();

	ShockGameLoop();

	return 0;
}

//------------------------------------------------------------------------------------
//		Handle Quit menu command/apple event.
//------------------------------------------------------------------------------------
void DoQuit(void)
{
//	if (AskToSave(1))
//	{
//		if (modeflag!=-1)
//			EndGame(false);
		gDone = true;
//	}
}

//--------------------------------------------------------------------
//  The main game loop for System Shock.
//--------------------------------------------------------------------
extern pascal void MousePollProc(void);
extern void pump_events(void);
extern long gShockTicks;

void ShockSetupLoop(void)
{
	// CC: Should unify all loops together into one master loop, like they used to be
	_new_mode = _current_loop = SETUP_LOOP;

	setup_init();
	setup_start();
	load_da_palette();		// KLC - added here.  Used to be in setup_start().

	/*if (startup_music)
	{
		start_music();
	}*/

	gr_clear(0xFF);

	splash_draw();

	while(_current_loop == SETUP_LOOP) {

		gShockTicks = TickCount();

		if (!(_change_flag&(ML_CHG_BASE<<1)))
			input_chk();
		
		// DG: at the beginning of each frame, get all the events from SDL
		pump_events();

		if (globalChanges)
		{
			if (_change_flag&(ML_CHG_BASE<<3))
				loopmode_switch(&_current_loop);
			chg_unset_flg(ML_CHG_BASE<<3);
		}

		if(_current_loop == SETUP_LOOP) {
			setup_loop();
		}

		chg_set_flg(_static_change);

		MousePollProc();		// update the cursor, was 35 times/sec originally

		// FIXME: should draw this bio bar again
		// status_bio_update();	// draw the biometer

		SDLDraw();
	}
}

void ShockGameLoop(void)
{
	gPlayingGame = TRUE;
	gDeadPlayerQuit = FALSE;
	gGameCompletedQuit = FALSE;

	gr_clear(0x0);
	load_da_palette();		// KLC - added here.  Used to be in setup_start().

	if (IsFullscreenWareOn())
	{
		fullscreen_start();
		_new_mode = _current_loop = FULLSCREEN_LOOP;
	}
	else
	{
		screen_start();											// Initialize the screen for slot view.
		_new_mode = _current_loop = GAME_LOOP;
	}

	while (gPlayingGame)
	{	
		gShockTicks = TickCount();

		if (!(_change_flag&(ML_CHG_BASE<<1)))
			input_chk();
		
		// DG: at the beginning of each frame, get all the events from SDL
		pump_events();

		if (globalChanges)
		{
			if (_change_flag&(ML_CHG_BASE<<3))
				loopmode_switch(&_current_loop);
			chg_unset_flg(ML_CHG_BASE<<3);
		}
		
		if (_current_loop == SETUP_LOOP)
			setup_loop();
		else if (_current_loop == AUTOMAP_LOOP)
			automap_loop();									// Do the fullscreen map loop.
		else {
			game_loop();										// Run the game!
		}
		
		chg_set_flg(_static_change);

		MousePollProc();		// update the cursor, was 35 times/sec originally
		status_bio_update();	// draw the biometer

		SDLDraw();
	}

	if(gGameCompletedQuit) {
		// FIXME: Revive the old cutscenes!
		printf("SHODAN has been defeated!\n");
	}

	/*
	// We're through playing now.
	uiHideMouse(NULL);
	loopmode_exit(_current_loop);
	status_bio_end();
     stop_music();											//KLC - add here to stop music at end game
	
	if (gDeadPlayerQuit)									// If we quit because the player was killed, show
	{																// the death movie.
		FSMakeFSSpec(gCDDataVref, gCDDataDirID, "Death", &fSpec);
		PlayCutScene(&fSpec, TRUE, TRUE);		
		gDeadPlayerQuit = FALSE;
	}

	if (gGameCompletedQuit)								// If we quit because the game was completed, show
	{																// the endgame movie.
		FSMakeFSSpec(gCDDataVref, gCDDataDirID, "Endgame", &fSpec);		
		PlayCutScene(&fSpec, TRUE, TRUE);
		gGameCompletedQuit = FALSE;

		PaintRect(&gMainWindow->portRect);
		ShowCursor();
		DoEndgameDlg();
	}

	closedown_game(TRUE);
	*/
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
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
		DebugString("SDL: Init failed");
	}

	SetupOffscreenBitmaps();

	// Point the renderer at the screen bytes
	gScreenRowbytes = drawSurface->w;
	gScreenAddress = drawSurface->pixels;
	gScreenWide = 640;
	gScreenHigh = 480;
	gActiveLeft = 0;
	gActiveTop = 0;
	gActiveWide = 640;
	gActiveHigh = 480;

	gr_init();

    gr_set_mode(GRM_640x480x8, TRUE);

    printf("Setting up screen and render contexts\n");

    // Open our window!

	window = SDL_CreateWindow(
		"System Shock - Shockolate 0.5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		grd_cap->w, grd_cap->h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

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
	gamePalette[255].a = 0xFF;

	SDL_SetPaletteColors(sdlPalette, gamePalette, 0, count);
	SDL_SetSurfacePalette(drawSurface, sdlPalette);
	SDL_SetSurfacePalette(offscreenDrawSurface, sdlPalette);
}

void SDLDraw()
{
	SDL_RenderClear(renderer);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, drawSurface);
	SDL_Rect srcRect = { 0, 0, gScreenWide, gScreenHigh };
	SDL_RenderCopy(renderer, texture, &srcRect, NULL);
	SDL_DestroyTexture(texture);
	SDL_RenderPresent(renderer);
}
