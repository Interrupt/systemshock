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
#include "InitMac.h"
#include "ShockBitmap.h"
#include "2d.h"
#include "simple.h"

WindowPtr	gMainWindow;
grs_screen	*screen;


void simp_startup()
{
	// Mac setup
	InitMac();
	CheckConfig();

	SetupWindows(&gMainWindow);								// setup everything
	SetupOffscreenBitmaps();			
	
	// 2D setup
	gr_init();
	gr_set_mode( GRM_640x480x8, true );
	screen = gr_alloc_screen( grd_cap->w, grd_cap->h );
	gr_set_screen( screen );
}


void simp_clear( void )
{
	gr_clear( 0xff );
}


void simp_shutdown( void )
{
	gr_close();
//   vga_rest_mode ();
}

