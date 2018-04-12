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
// simple test code for 2d library 

#include <2d.h>
#include "fix.h"
#include "lg.h"
#include <stdio.h>

//#include <SDL2/SDL.h>
#include <sdl.h>

long		gScreenRowbytes;
CTabHandle	gMainColorHand;
Ptr			gScreenAddress;

// prototypes
void SetVertexLinear(grs_vertex **points);
void SetVertexFloor(grs_vertex **points);
void SetVertexWall(grs_vertex **points);
void SetVertexPerHScan(grs_vertex **points);
void SetVertexPerVScan(grs_vertex **points);
void WaitKey(void);
void SetSDLPalette(int index, int count, uchar *pal);

#define make_vertex(_vertex,_x,_y,_u,_v,_w,_i) \
	 _vertex.x = fix_make(_x,0), \
	 _vertex.y = fix_make(_y,0), \
	 _vertex.u = fix_make(_u,0),             \
	 _vertex.v = fix_make(_v,0),_vertex.w = _w, _vertex.i = _i; 

#define clear_color 400

char test_clut[111] = { 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,54,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 0,0,0,0,0,0,0,0,0,0,
						 80
						};


uchar pal_buf[768];
uchar bitmap_buf[17000];
uchar shade_buf[4096];

SDL_Window* window;
SDL_Surface* drawSurface;

int main(void)
{
	DebugStr("Starting Test");

	FILE        *fp;
	grs_screen  *screen;
	char        *fd;
	grs_bitmap  bm;
	grs_vertex  v0,v1,v2,v3;
	grs_vertex  *points[4];
	grs_canvas	canvas;

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		DebugStr("SDL: Init failed");
		return 1;
	}

	window = SDL_CreateWindow(
		"System Shock - SimpleMain Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		640, 480, SDL_WINDOW_SHOWN);

	SDL_RaiseWindow(window);
	
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

	drawSurface = SDL_CreateRGBSurface(0, 640, 480, 8, 0, 0, 0, 0);
	if(!drawSurface) {
		DebugStr("SDL: Failed to create draw surface");
		return 1;
	}

	gScreenRowbytes = drawSurface->w;
	gScreenAddress = drawSurface->pixels;

	gr_init();
	gr_set_mode (GRM_640x480x8, TRUE);
	screen = gr_alloc_screen (drawSurface->w, drawSurface->h);
	gr_set_screen (screen);

	// HAX: Why aren't the canvas rows set by default from gr_set_screen?
	//grd_bm.row = drawSurface->w;

	DebugStr("Opening test.img");
	if(fp = fopen("test.img","rb")) {
		fread (bitmap_buf, 1, 16412, fp);
		fclose (fp);
	} else {
		DebugStr("Open failed");
		return 1;
	}

	bm = * (grs_bitmap *) bitmap_buf;
   	gr_init_bm(&bm, (uchar *) bitmap_buf+28, BMT_FLAT8, 0, 128, 128);

	DebugStr("Opening test.pal");
	if(fp = fopen("test.pal","rb")) {
		fread (pal_buf, 1, 768, fp);
		fclose (fp);
	} else {
		DebugStr("Open failed");
		return 1;
	}

	DebugStr("Setting palette");
	gr_set_pal(0, 256, pal_buf);

	SetSDLPalette(0, 256, pal_buf);

	DebugStr("Alloc Ipal");
	gr_alloc_ipal();
	gr_init_blend(1);
	gr_clear(0x0);
	WaitKey();

	DebugStr("Setting points");
	points[0] = &v0;
	points[1] = &v1;
	points[2] = &v2;
	points[3] = &v3;

	DebugStr("Opening test.shd");
	fp = fopen("test.shd","rb");
	fread (shade_buf, 1, 4096, fp);
	fclose (fp);

	DebugStr("Set Light Table");	
	gr_set_light_tab(shade_buf);


	DebugStr("Starting Drawing");
// ==
	// linear
	SetVertexLinear(points);
	gr_poly(2, 4, points);
	WaitKey();
	gr_per_umap(&bm, 4, points);
	WaitKey();
	gr_clut_per_umap(&bm, 4, points, test_clut);
	WaitKey();
	gr_lit_per_umap(&bm, 4, points);

	WaitKey();
	gr_clear(clear_color);
	WaitKey();
	
	// wall 
	SetVertexWall(points);
	gr_poly(2, 4, points);
	WaitKey();
	gr_per_umap(&bm, 4, points);
	WaitKey();
	gr_lit_per_umap(&bm, 4, points);

	WaitKey();
	gr_clear(clear_color);
	WaitKey();

	// floor
	SetVertexFloor(points);
	gr_poly(2, 4, points);
	WaitKey();
	gr_per_umap(&bm, 4, points);
	WaitKey();
	gr_lit_per_umap(&bm, 4, points);

	WaitKey();
	WaitKey();
	WaitKey();
	WaitKey();
	gr_clear(clear_color);
	WaitKey();


	// perspective(hscan)
	SetVertexPerHScan(points);
	gr_poly(2, 4, points);
	WaitKey();
	gr_per_umap(&bm, 4, points);
	WaitKey();
	gr_lit_per_umap(&bm, 4, points);

	WaitKey();
	gr_clear(clear_color);
	WaitKey();

	// perspective(vscan)
	SetVertexPerVScan(points);
	gr_poly(2, 4, points);
	WaitKey();
	gr_per_umap(&bm, 4, points);
	WaitKey();
	gr_lit_per_umap(&bm, 4, points);

	WaitKey();
	gr_clear(clear_color);
	WaitKey();
		

// ===== test code     
	SetVertexPerHScan(points);
	SetVertexLinear(points);

	gr_set_fill_type(FILL_SOLID);
	gr_set_fill_parm(33);

	gr_per_umap(&bm, 4, points);
	gr_clut_per_umap(&bm, 4, points, test_clut);
	gr_lit_per_umap(&bm, 4, points);

	WaitKey();

	for (int i=0; i<100; i++) {
		gr_clear(i);
		gr_per_umap(&bm, 4, points);
		gr_clut_per_umap(&bm, 4, points, test_clut);
		gr_lit_per_umap(&bm, 4, points);

		SDL_UpdateWindowSurface( window );
		SDL_Delay(2);
	}

//	time = TickCount()-time;
//	NumToString(time,str);
//  DebugStr(str);  
		 
 
	gr_close();
}

void SetSDLPalette(int index, int count, uchar *pal)
{
	SDL_Color gamePalette[256];
	for(int i = index; i < count; i++) {
		gamePalette[index+i].r = *pal++;
		gamePalette[index+i].g = *pal++;
		gamePalette[index+i].b = *pal++;
		gamePalette[index+i].a = 0xFF;
	}

	SDL_Palette* sdlPalette = SDL_AllocPalette(count);
	SDL_SetPaletteColors(sdlPalette, gamePalette, 0, count);
	SDL_SetSurfacePalette(drawSurface, sdlPalette);
}

void WaitKey(void)
{
	SDL_Surface* screenSurface = SDL_GetWindowSurface( window );
	SDL_BlitSurface(drawSurface, NULL, screenSurface, NULL);
  	SDL_UpdateWindowSurface(window);

  	SDL_PumpEvents();
	SDL_Delay(200);
}

void SetVertexLinear(grs_vertex **points)
 {
	make_vertex((*(points[0])),100,   100,    0,    0,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
	make_vertex((*(points[1])),200,   120,    128,    0,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
	make_vertex((*(points[2])),200,   180,    128,    128,    fix_div(FIX_UNIT,fix_make(10,0)), 16*FIX_UNIT-1);
	make_vertex((*(points[3])),100,   200,    0,    128,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
 }
 

void SetVertexFloor(grs_vertex **points)
 {
	make_vertex((*(points[0])),100,   100,    0,    0,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
	make_vertex((*(points[1])),1000,   100,    128,    0,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
	make_vertex((*(points[2])),1080,   300,    128,    128,    fix_div(FIX_UNIT,fix_make(20,0)), 16*FIX_UNIT-1);
	make_vertex((*(points[3])),120,   300,    0,    128,    fix_div(FIX_UNIT,fix_make(20,0)), 0);
 }
 

void SetVertexWall(grs_vertex **points)
 {
	make_vertex((*(points[0])),100,   100,    0,    0,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
	make_vertex((*(points[1])),200,   120,    128,    0,    fix_div(FIX_UNIT,fix_make(20,0)), 0);
	make_vertex((*(points[2])),200,   180,    128,    128,    fix_div(FIX_UNIT,fix_make(20,0)), 16*FIX_UNIT-1);
	make_vertex((*(points[3])),100,   200,    0,    128,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
 }
 

void SetVertexPerHScan(grs_vertex **points)
 {
	make_vertex((*(points[0])),100,   100,    0,    0,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
	make_vertex((*(points[1])),200,   120,    128,    0,    fix_div(FIX_UNIT,fix_make(12,0)), 0);
	make_vertex((*(points[2])),180,   200,    128,    128,    fix_div(FIX_UNIT,fix_make(20,0)), 16*FIX_UNIT-1);
	make_vertex((*(points[3])),105,   200,    0,    128,    fix_div(FIX_UNIT,fix_make(14,0)), 0);
 }
 
void SetVertexPerVScan(grs_vertex **points)
 {
	make_vertex((*(points[0])),100,   100,    0,    0,    fix_div(FIX_UNIT,fix_make(10,0)), 0);
	make_vertex((*(points[1])),200,   120,    128,    0,    fix_div(FIX_UNIT,fix_make(20,0)), 0);
	make_vertex((*(points[2])),180,   200,    128,    128,    fix_div(FIX_UNIT,fix_make(20,0)), 16*FIX_UNIT-1);
	make_vertex((*(points[3])),105,   200,    0,    128,    fix_div(FIX_UNIT,fix_make(12,0)), 0);
 }
 
