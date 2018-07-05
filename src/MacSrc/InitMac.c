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
//#include <Palettes.h>
//#include <GestaltEqu.h>
//#include <Movies.h>

#include "Shock.h"
#include "InitMac.h"
#include "ShockBitmap.h"
//#include "MacTune.h"
#include <SDL.h>

#include <Carbon/Carbon.h>

//--------------------
//  Globals
//--------------------
#ifndef __MWERKS__
//QDGlobals	qd;
#endif
Handle			gExtraMemory = nil;
//ColorSpec 		*gOriginalColors;
unsigned long	gRandSeed;
short				gMainVRef;
//CursHandle		gWatchCurs;
short				gOriginalDepth = -1;
short				gLastAlertDepth = -1;
short				gStartupDepth;
Ptr				gScreenAddress;
long				gScreenRowbytes;
short				gScreenWide, gScreenHigh;
short				gActiveWide, gActiveHigh;
short				gActiveLeft, gActiveTop;
Rect				gActiveArea, gOffActiveArea;
Boolean			gIsPowerPC = false;
long				gDataDirID;
short				gDataVref;
long				gCDDataDirID;
short				gCDDataVref;
long				gAlogDirID;
short				gAlogVref;
long				gBarkDirID;
short				gBarkVref;
Boolean			gMenusHid;

//---------------------------
//  Externs
//---------------------------
void status_bio_update(void);
extern uchar gBioInited;
pascal void MousePollProc(void);


//---------------------------
//  Internal Prototypes
//---------------------------
void Cleanup(void);

//---------------------------
//  Time Manager routines and globals
//---------------------------
TimerUPP		pShockTicksPtr;				// Globals for the Shock "tickcount" TM task.
ShockTask		pShockTicksTask;			// It increments gShockTicks 280 times per second.
long				gShockTicks;
long 				*tmd_ticks;

//------------------------------------------------------------------------------------
//		Initialize the Macintosh managers.
//------------------------------------------------------------------------------------
void InitMac(void)
{
	INFO("Starting Shockolate");

	// Get a random seed
	gRandSeed = TickCount();
	gRandSeed += TickCount()<<8;

	InstallShockTimers(); // needed for the tick pointer
}

//------------------------------------------------------------------------------------
//		Get a resource and fail correctly if it can't be loaded.
//------------------------------------------------------------------------------------
Handle GetResourceFail(long id, short num)
{
	Handle 	h;
	
	h = GetResource(id, num);
	if (h) return(h);
	
#if 1
	STUB("If GetResource() and friends are relevant after all, implement this..")
#else

	// At this point GetResource failed, figure out why.
	SetResLoad(false);
	h = GetResource(id, num);
	SetResLoad(true);
	
	if (gExtraMemory)
		DisposHandle(gExtraMemory);
	
	if (h) 
		ErrorDie(1);		// resource is there, must be a memory problem
	else
		ErrorDie(3);		// resource not there, somethings bad
#endif
	return (nil);
}


//------------------------------------------------------------------------------------
//  Startup the SystemShock timer.
//------------------------------------------------------------------------------------
void InstallShockTimers(void)
{
	gShockTicks = 0;
	tmd_ticks = &gShockTicks;
}

//------------------------------------------------------------------------------------
//  Remove the SystemShock timer.
//------------------------------------------------------------------------------------
void RemoveShockTimers(void)
{
#if 1
	STUB("if the timer is still used, remove it here..")
#else
	RmvTime((QElemPtr)&pShockTicksTask);					// Stop the Shock ticks task
	DisposeRoutineDescriptor(pShockTicksPtr);					// Dispose its UPP
#endif
}

//------------------------------------------------------------------------------------
//  Timer tick callback.
//------------------------------------------------------------------------------------
Uint32 TimerTickCallback(Uint32 interval, void *param)
{
	gShockTicks++;
	return interval;
}
 
//------------------------------------------------------------------------------------
//  Display an alert using the str# resource with index strignum, then die.
//------------------------------------------------------------------------------------
void ErrorDie(short stringnum)
{
	/*if (gExtraMemory)
		DisposHandle(gExtraMemory);	// free our extra space
 
 	StringAlert(stringnum);
	CleanupAndExit();*/
}

//------------------------------------------------------------------------------------
// 	Display an alert using the str# resource with index strignum
//------------------------------------------------------------------------------------
void StringAlert(short stringnum)
{
	/*Str255		message, explain;
	
	InitCursor();
	GetIndString(message, 1000, stringnum);
	GetIndString(explain, 1001, stringnum);
	ParamText(message, explain, "", "");
	
	if (*explain)
		StopAlert(1001, nil);
	else
		StopAlert(1000, nil);*/
}

#pragma mark -
//------------------------------------------------------------------------------------
//  Close all our resources, then quit.
//------------------------------------------------------------------------------------
void Cleanup(void)
{
	/*GDHandle	devhandle;

	MacTuneShutdown();
	RemoveShockTimers();

	snd_kill_all_samples();
	snd_shutdown();
	
	if (gOriginalDepth != -1)											// If color depth was changed at beginning of app,
	{																				// then switch it back to the original.
		devhandle = GetMainDevice();
		if (devhandle)
			if (HasDepth(devhandle, gOriginalDepth, 0, 0))
				SetDepth(devhandle, gOriginalDepth, 0, 0);
	}
	else
		CleanupPalette();													// Else switch back to original 8-bit palette.
	
	gr_close();
	mouse_shutdown();
	kb_shutdown();
	ResTerm();*/
}

//------------------------------------------------------------------------------------
//  Normal cleanup when the program quits.
//------------------------------------------------------------------------------------
void CleanupAndExit(void)
{
	/*Cleanup();
	ExitToShell();*/
}

