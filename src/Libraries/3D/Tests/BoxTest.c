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
/*
 * $Source: n:/project/lib/src/3d/test/RCS/bitmap.c $
 * $Revision: 1.1 $
 * $Author: kaboom $
 * $Date: 1993/07/28 21:03:55 $
 */

#define __FAUXREND_SRC

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "2d.h"
#include "2dRes.h"
#include "3d.h"
#include "lg.h"

#include <Carbon/Carbon.h>
#include <sdl.h>
#include <stdio.h>

#define pitch        tx
#define bank         tz
#define head         ty

long fr_clear_color = 0xff;

long		gScreenRowbytes;
Ptr			gScreenAddress;

// prototypes
void test_3d(void);
void setup_box_side(fix boxsize, int whichside, g3s_phandle *trans_p);


g3s_vector viewer_position;
g3s_angvec viewer_orientation;

#define coor(val) (fix_make((eye[val]>>MAP_SH),(eye[val]&MAP_MK)<<MAP_MS))
#define ang(val)  (eye[val])
#define setvec(vec,x,y,z) {vec.xyz[0] = x; vec.xyz[1] = y; vec.xyz[2] = z;}

#define box_front 0
#define box_back 1
#define box_left 2
#define box_right 3
#define box_top 4
#define box_bottom 5

SDL_Window* window;
SDL_Surface* drawSurface;

void SetupSDL() {
	window = SDL_CreateWindow(
		"System Shock - SimpleMain Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		640, 480, SDL_WINDOW_SHOWN);

	SDL_RaiseWindow(window);
	
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

	drawSurface = SDL_CreateRGBSurface(0, 640, 480, 8, 0, 0, 0, 0);
	if(!drawSurface) {
		DebugString("SDL: Failed to create draw surface");
		return;
	}
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

void SDLDraw(void)
{
	SDL_Surface* screenSurface = SDL_GetWindowSurface( window );
	SDL_BlitSurface(drawSurface, NULL, screenSurface, NULL);
  	SDL_UpdateWindowSurface(window);
  	SDL_PumpEvents();
}


// setup polygon for a box side (0-5, front, back, left, right, top, bottom)
void setup_box_side(fix boxsize, int whichside, g3s_phandle *trans_p)
 {
	g3s_vector cur_vec;
	
	switch (whichside)
	 {
	 	case box_front:		setvec(cur_vec, -boxsize,-boxsize,-boxsize); 
	 										trans_p[0]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,-boxsize,-boxsize); 
	 										trans_p[1]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,boxsize,-boxsize); 
	 										trans_p[2]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, -boxsize,boxsize,-boxsize); 
	 										trans_p[3]=g3_transform_point(&cur_vec);
	 										
											trans_p[0]->uv.u = 0; trans_p[0]->uv.v = 0; 
											trans_p[1]->uv.u = 256; trans_p[1]->uv.v = 0; 
											trans_p[2]->uv.u = 256; trans_p[2]->uv.v = 256; 
											trans_p[3]->uv.u = 0; trans_p[3]->uv.v = 256; 
	 										break;
	 	
	 	case box_back: 		setvec(cur_vec, -boxsize,-boxsize,boxsize); 
	 										trans_p[3]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,-boxsize,boxsize); 
	 										trans_p[2]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,boxsize,boxsize); 
	 										trans_p[1]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, -boxsize,boxsize,boxsize); 
	 										trans_p[0]=g3_transform_point(&cur_vec);
	 										
											trans_p[3]->uv.u = 0; trans_p[3]->uv.v = 0; 
											trans_p[2]->uv.u = 256; trans_p[2]->uv.v = 0; 
											trans_p[1]->uv.u = 256; trans_p[1]->uv.v = 256; 
											trans_p[0]->uv.u = 0; trans_p[0]->uv.v = 256; 
	 										break;

	 	case box_left:		setvec(cur_vec, -boxsize,-boxsize,-boxsize); 
	 										trans_p[3]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, -boxsize,-boxsize,boxsize); 
	 										trans_p[2]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, -boxsize,boxsize,boxsize); 
	 										trans_p[1]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, -boxsize,boxsize,-boxsize); 
	 										trans_p[0]=g3_transform_point(&cur_vec);
	 										
											trans_p[3]->uv.u = 0; trans_p[3]->uv.v = 0; 
											trans_p[2]->uv.u = 256; trans_p[2]->uv.v = 0; 
											trans_p[1]->uv.u = 256; trans_p[1]->uv.v = 256; 
											trans_p[0]->uv.u = 0; trans_p[0]->uv.v = 256; 
	 										break;
	 	case box_right:		setvec(cur_vec, boxsize,-boxsize,-boxsize); 
	 										trans_p[0]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,-boxsize,boxsize); 
	 										trans_p[1]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,boxsize,boxsize); 
	 										trans_p[2]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,boxsize,-boxsize); 
	 										trans_p[3]=g3_transform_point(&cur_vec);
	 										
											trans_p[0]->uv.u = 0; trans_p[0]->uv.v = 0; 
											trans_p[1]->uv.u = 256; trans_p[1]->uv.v = 0; 
											trans_p[2]->uv.u = 256; trans_p[2]->uv.v = 256; 
											trans_p[3]->uv.u = 0; trans_p[3]->uv.v = 256; 
	 										break;
	 	case box_top:			setvec(cur_vec, -boxsize,-boxsize,-boxsize); 
	 										trans_p[0]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, -boxsize,-boxsize,boxsize); 
	 										trans_p[1]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,-boxsize,boxsize); 
	 										trans_p[2]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,-boxsize,-boxsize); 
	 										trans_p[3]=g3_transform_point(&cur_vec);
	 										
											trans_p[0]->uv.u = 0; trans_p[0]->uv.v = 0; 
											trans_p[1]->uv.u = 256; trans_p[1]->uv.v = 0; 
											trans_p[2]->uv.u = 256; trans_p[2]->uv.v = 256; 
											trans_p[3]->uv.u = 0; trans_p[3]->uv.v = 256; 
	 										break;
	 	case box_bottom:	setvec(cur_vec, -boxsize,boxsize,-boxsize); 
	 										trans_p[3]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, -boxsize,boxsize,boxsize); 
	 										trans_p[2]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,boxsize,boxsize); 
	 										trans_p[1]=g3_transform_point(&cur_vec);
	 										setvec(cur_vec, boxsize,boxsize,-boxsize); 
	 										trans_p[0]=g3_transform_point(&cur_vec);
	 										
											trans_p[3]->uv.u = 0; trans_p[3]->uv.v = 0; 
											trans_p[2]->uv.u = 256; trans_p[2]->uv.v = 0; 
											trans_p[1]->uv.u = 256; trans_p[1]->uv.v = 256; 
											trans_p[0]->uv.u = 0; trans_p[0]->uv.v = 256; 
	 										break;
	 }
 }
 

#define build_fix_angle(ang) ((65536*(ang))/360)
#define num_bitmaps 5

int	face_value[6] = {0,1,2,3,4,1};

void test_3d(void)
 {
	g3s_phandle 	trans[8];
	int 					i,j,c=0;
	grs_bitmap 		bm[num_bitmaps];
	EventRecord 	evt;
	grs_screen 		*screen;
	g3s_vector		vec;
	g3s_angvec		ang;
	int						dx = 0, dy = 0, dz = 0;
	grs_canvas 		*off_canvas;
	//Handle				palRes,bmres[num_bitmaps];
	FrameDesc			*fd;
	fix						vx,vy,vz;
	int						ddx = 0, ddy = 0, ddz = 0;
	long					time,frames;
	Str255				str;
	Rect					r;

	uchar pal_buf[768];
	uchar bitmap_buf[17000];

	DebugString("Opening test.pal");
	FILE* fp;
	if(fp = fopen("test.pal","rb")) {
		fread (pal_buf, 1, 768, fp);
		fclose (fp);
	} else {
		DebugString("Open failed");
		return;
	}

	DebugString("Opening test.img");
	if(fp = fopen("test.img","rb")) {
		fread (bitmap_buf, 1, 16412, fp);
		fclose (fp);
	} else {
		DebugString("Open failed");
		return;
	}
	
	/*printf("Loading pal.dat\n");
	palRes = GetResource('src/Libraries/3D/pal.dat ',1000);
	HLock(palRes);*/
	
	/*for (i=0; i<num_bitmaps; i++)
	 {
	 	printf("Loading bit.dat\n");
		bmres[i] = GetResource('src/Libraries/3D/bit.dat',1000+i);

		printf("Getting bits\n");
		HLock(bmres[i]);
		fd = (FrameDesc *) *bmres[i];
		fd->bm.bits = (uchar *)(fd+1);
		bm[i] = fd->bm;
	 }*/

	SetupSDL();

	gScreenRowbytes = drawSurface->w;
	gScreenAddress = drawSurface->pixels;
	
	printf("Setting up screen\n");
	gr_init();

	for(int i = 0; i < num_bitmaps; i++) {
   		gr_init_bm(&bm[i], (uchar *) bitmap_buf+28, BMT_FLAT8, 0, 128, 128);
   	}

	gr_set_mode(GRM_640x480x8, TRUE);
	screen = gr_alloc_screen(640,480);
	gr_set_screen (screen);

	gr_set_pal(0, 256, pal_buf);
	gr_alloc_ipal();
	gr_set_fcolor(fr_clear_color);

	SetSDLPalette(0, 256, pal_buf);

	gr_clear(0x0);
	
	printf("Setting up canvas\n");
	off_canvas = gr_alloc_canvas(BMT_FLAT8,640,480);
	gr_set_canvas(off_canvas);
	gr_clear(fr_clear_color);
	gr_set_canvas(grd_screen_canvas);
	
	g3_init(80,AXIS_RIGHT,AXIS_DOWN,AXIS_IN);
	
	viewer_position.gX = viewer_position.gY = 0;
	viewer_position.gZ = fix_make(-60,0);
	viewer_orientation.head = viewer_orientation.bank = viewer_orientation.head = 0;

	vec.gX = 0;
	vec.gY = fix_make(-40,0);
	vec.gZ = fix_make(5,0);

	vx = 0xc000;
	vy = 0xc000;
	vz = -0xc000;
	ddx = 2;
	ddy = 2;
	
	dx = 50;
	dy = 60;
	
	frames = 0;
	time = 1;
	do
	 {	 	
	 	time++;

	 	uint8* keyboard;
    	keyboard = SDL_GetKeyboardState(NULL);

		//gr_set_canvas(off_canvas);
		gr_clear(fr_clear_color);     

		g3_start_frame();
		g3_set_view_angles(&viewer_position,&viewer_orientation,ORDER_YXZ,g3_get_zoom('X',build_fix_angle(70),640,480));
		

		if (keyboard[SDL_SCANCODE_LEFT]) dz -= ddx;
		if (keyboard[SDL_SCANCODE_RIGHT]) dz += ddx;

		//if (keyboard[SDL_SCANCODE_LEFT]) vec.gZ -= vz;
		//if (keyboard[SDL_SCANCODE_RIGHT]) vec.gZ += vz;

		if (keyboard[SDL_SCANCODE_UP]) vec.gX -= vx;
		if (keyboard[SDL_SCANCODE_DOWN]) vec.gX += vx;

	 	/*switch (evt.message & charCodeMask)
	 	 {
	 	 	case '8': vec.gY -= vy; break;
	 	 	case '2': vec.gY += vy; break; 
	 	 	case '6': vec.gX += vx; break;
	 	 	case '4': vec.gX -= vx; break;
	 	 	case '7': vec.gZ -= vz; break;
	 	 	case '9': vec.gZ += vz; break;
	 	 	
	 	 	case 'a': dx -= ddx; break;
	 	 	case 'd': dx += ddx; break;
	 	 	case 'w': dy -= ddx; break;
	 	 	case 's': dy += ddx; break;
	 	 	case 'q': dz -= ddx; break;
	 	 	case 'e': dz += ddx; break;
	 	 }*/
		 
		
/*		dx += ddx;
		dy += ddy;
		dz += ddz;
		
		vec.gX += vx;
		vec.gY += vy;
		vec.gZ += vz;
		
		if ((vec.gZ<fix_make(0,0)) || (vec.gZ>fix_make(130,0))) vz = -vz;
		if ((vec.gX<fix_make(-45,0)) || (vec.gX>fix_make(45,0))) {vx = -vx; ddx = -ddx;}
		if ((vec.gY<fix_make(-35,0)) || (vec.gY>fix_make(35,0))) {vy = -vy; ddy = -ddy;}*/
		 
		g3_start_object_angles_xyz(&vec,build_fix_angle(dy), build_fix_angle(dx), build_fix_angle(dz), ORDER_YXZ);
		for (i=box_front; i<=box_bottom; i++)
		 {
			setup_box_side(fix_make(10,0), i, trans);
			g3_check_and_draw_tmap_quad_tile(trans, &bm[face_value[i]],1,1);
//			g3_check_and_draw_poly((10*i)+30,a4,trans);
		 } 
		g3_end_object();
		g3_end_frame();

		//gr_set_canvas(grd_screen_canvas);
		//gr_bitmap(&off_canvas->bm,0,0);	

/*	 	frames++;
		if (TickCount()-time>=60L)
		 {
			NumToString(frames,str);
			SetRect(&r,0,0,32,20);
			EraseRect(&r);
			MoveTo(2,10);
			DrawString(str);
			
			frames = 0;
			time = TickCount();
		 }*/

		SDLDraw();
		SDL_Delay(30);
	 }
	while (!Button());
     
	g3_shutdown();
  gr_close();
}

void main() {
	test_3d();
}