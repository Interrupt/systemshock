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
//		InitMac.h	-	Initialize Mac toolbox managers and setup the application's globals.
//
//====================================================================================

// How many tick passed since game startup
extern uint32_t gShockTicks;

extern char					*gScreenAddress;
extern long					gScreenRowbytes;
extern short				gScreenWide, gScreenHigh;
extern short				gActiveWide, gActiveHigh;
extern short				gActiveLeft, gActiveTop;

//--------------------
//  Prototypes
//--------------------
void InitMac(void);
void CleanupAndExit(void);
void InstallShockTimers(void);
