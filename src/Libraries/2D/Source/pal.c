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
 * $Source: n:/project/lib/src/2d/RCS/pal.c $
 * $Revision: 1.8 $
 * $Author: lmfeeney $
 * $Date: 1994/06/18 04:05:26 $
 *
 * Routines and data for non-hardware-dependent palette control.
 *
 * This file is part of the 2d library.
 *
 * $Log: pal.c $
 * Revision 1.8  1994/06/18  04:05:26  lmfeeney
 * added set palette with gamma correct
 * 
 * Revision 1.7  1993/10/19  09:51:43  kaboom
 * Replaced #include "grd.h> with new headers split from grd.h.
 * 
 * Revision 1.6  1993/10/08  01:16:18  kaboom
 * Changed quotes in #include liness to angle brackets for Watcom problem.
 * 
 * Revision 1.5  1993/07/12  23:31:37  kaboom
 * Now bounds checks number of palette entries.
 * 
 * Revision 1.4  1993/04/29  19:07:29  kaboom
 * Changed include of old gr.h to grdev.h.
 * 
 * Revision 1.3  1993/02/08  11:58:05  kaboom
 * Fixed bug in gr_set_pal -- wasn't adjusting for start of pal.
 * 
 * Revision 1.2  1993/02/04  17:42:48  kaboom
 * Changed includes.
 * 
 * Revision 1.1  1993/01/29  17:27:29  kaboom
 * Initial revision
 */

#include <string.h>
#include "rgb.h"
#include "grdev.h"
#include "scrdat.h"
#include "fix.h"
#include "lg.h"

//gamma param not used here; see SetSDLPalette() in Shock.c
void gr_set_gamma_pal (int start, int n, fix gamma)
{
  gr_set_screen_pal (start, n, grd_pal+3*start);
}

/* copy user's palette into shadow palette, then set real palette. */
void gr_set_pal (int start, int n, uchar *pal_data)
{
   int i;
   uchar r,g,b;            /* red,green,blue values */

   if (n <= 0)
      return;

   LG_memcpy (grd_pal+3*start, pal_data, 3*n);
   for (i=start; i<start+n; i++) {
      r = grd_pal[3*i];
      g = grd_pal[3*i+1];
      b = grd_pal[3*i+2];
      grd_bpal[i] = gr_bind_rgb (r, g, b);
   }
   gr_set_screen_pal (start, n, grd_pal+3*start);
}

/* copy the shadow palette to a destination buffer. */
void gr_get_pal (int start, int n, uchar *pal_data)
{
   if (n <= 0)
      return;
   LG_memcpy (pal_data, grd_pal+3*start, 3*n);
}
