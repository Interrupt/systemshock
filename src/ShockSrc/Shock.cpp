/*

Copyright (C) 1994-1995 Looking Glass Technologies, Inc.
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

//--------------------
//  Includes
//--------------------

extern "C" {
#include "Shock.h"
#include "InitMac.h"
#include "OpenGL.h"
#include "Prefs.h"
#include "ShockBitmap.h"
#include "ShockHelp.h"

#include "amaploop.h"
#include "hkeyfunc.h"
#include "init.h"
#include "mainloop.h"
#include "setup.h"
#include "fullscrn.h"
#include "status.h"
#include "map.h"
#include "gr2ss.h"
#include "frflags.h"
#include "version.h"

#include "Modding.h"

//--------------------
//  Prototypes
//--------------------
extern void inv_change_fullscreen(uchar on);
}

//--------------------
//  Globals
//--------------------
bool gPlayingGame; //¥¥¥ Temp
bool gDeadPlayerQuit;

grs_screen *cit_screen;
int num_args;
char **arg_values;

extern grs_screen *svga_screen;
extern frc *svga_render_context;

//------------------------------------------------------------------------------------
//		Main function.
//------------------------------------------------------------------------------------
int main(int argc, char **argv) {
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

    // see Prefs.c
    CreateDefaultKeybindsFile(); // only if it doesn't already exist
    // even if keybinds file still doesn't exist, defaults will be set here
    LoadHotkeyKeybinds();
    LoadMoveKeybinds();

    // Process some startup arguments

    bool show_splash = !CheckArgument("-nosplash");

    // CC: Modding support! This is so exciting.

    ProcessModArgs(argc, argv);

    // Initialize

    init_all();
    setup_init();

    gPlayingGame = TRUE;
    gDeadPlayerQuit = FALSE;

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

bool CheckArgument(const char *arg) {
    if (arg == nullptr)
        return false;

    for (int i = 1; i < num_args; i++) {
        if (strcmp(arg_values[i], arg) == 0) {
            return true;
        }
    }

    return false;
}
