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

#include <SDL.h>

//--------------------
//  Function Prototypes
//--------------------
int main(int argc, char **argv);

void InitSDL();
void SetSDLPalette(int index, int count, uchar *pal);
void SDLDraw();
void CaptureMouse(bool capture);
bool CheckArgument(const char *name);

//--------------------
// Public Globals
//--------------------

extern unsigned long gRandSeed;

extern char *gScreenAddress;
extern long gScreenRowbytes;
extern short gScreenWide, gScreenHigh;

extern grs_screen *cit_screen;
extern SDL_Window *window;
