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
// InitMac.c - Initialize Mac toolbox managers and setup the application's globals.

//--------------------
//  Includes
//--------------------
#include <SDL.h>

#include "InitMac.h"
#include "Shock.h"
#include "ShockBitmap.h"
#include "shockolate_version.h"

//  Globals

intptr_t *gScreenAddress;
int32_t gScreenRowbytes;
int32_t gScreenWide, gScreenHigh;

//  Time Manager routines and globals

uint32_t gShockTicks;
uint32_t *tmd_ticks;

void InitMac(void) {
    INFO("Starting %s", SHOCKOLATE_VERSION);
    InstallShockTimers(); // needed for the tick pointer
}

void InstallShockTimers(void) {
    gShockTicks = 0;
    tmd_ticks = &gShockTicks;
}

void CleanupAndExit(void) {
    /*Cleanup();
    ExitToShell();*/
}
