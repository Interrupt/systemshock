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
//		ShockBitmap.c	-	Manages off-screen bitmaps and palettes.
//
//====================================================================================

#include <stdbool.h>

#include <Carbon/Carbon.h>
#include <sdl.h>

//--------------------
//  Types
//--------------------
typedef struct 
{
	Ptr		Address;
	long		RowBytes;
	GrafPtr	bits;
	Rect		bounds;
	bool	Color;
	
	BitMap	BWBits;			// B&W port & map
	GrafPort BWPort;
	
	CGrafPort	CPort;		// color port
	
	Handle	OrigBits;
	
} ShockBitmap;


//--------------------
//  Globals
//--------------------
extern PixMapHandle 		gScreenPixMap;
extern CTabHandle			gMainColorHand;
extern bool				gChangedColors;
//extern ShockBitmap			gMainOffScreen;

extern SDL_Surface* drawSurface;
extern SDL_Surface* offscreenDrawSurface;


//--------------------
//  Prototypes
//--------------------
void SetupOffscreenBitmaps(void);
void ResetCTSeed(void);
void RememberSeed(void);
void FixPalette(void);
void NewShockBitmap(ShockBitmap *theMap, short width, short height, bool color);
void FreeShockBitmap(ShockBitmap *theMap);
Handle Build8PixMap(CGrafPtr theCGrafPtr,short width, short height);
short CurScreenDepth(void);
void CheckBitDepth(void);
void CleanupPalette(void);
void LoadPictShockBitmap(ShockBitmap *theMap, short PictID);

void SetupTitleScreen(void);
int DoShockTitleButtons(Point mousePt);
