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
//		InitMac.c	-	Initialize Mac toolbox managers and setup the application's globals.
//
//====================================================================================

//--------------------
//  Includes
//--------------------
#include <SDL.h>

#include "InitMac.h"
#include "Shock.h"
#include "ShockBitmap.h"
#include "shockolate_version.h"

//--------------------
//  Globals
//--------------------
char *gScreenAddress;
long gScreenRowbytes;
short gScreenWide, gScreenHigh;
short gActiveWide, gActiveHigh;
short gActiveLeft, gActiveTop;

//---------------------------
//  Time Manager routines and globals
//---------------------------
uint32_t gShockTicks;
uint32_t *tmd_ticks;

//------------------------------------------------------------------------------------
//		Initialize the Macintosh managers.
//------------------------------------------------------------------------------------
void InitMac(void) {
    INFO("Starting %s", SHOCKOLATE_VERSION);
    InstallShockTimers(); // needed for the tick pointer
}

//------------------------------------------------------------------------------------
//  Startup the SystemShock timer.
//------------------------------------------------------------------------------------
void InstallShockTimers(void) {
    gShockTicks = 0;
    tmd_ticks = &gShockTicks;
}

//------------------------------------------------------------------------------------
//  Normal cleanup when the program quits.
//------------------------------------------------------------------------------------
void CleanupAndExit(void) {
    /*Cleanup();
    ExitToShell();*/
}
