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
 * $Source: n:/project/lib/src/2d/RCS/fl8clear.c $
 * $Revision: 1.3 $
 * $Author: kaboom $
 * $Date: 1993/10/19 09:50:18 $
 *
 * Routines for clearing a flat 8 canvas.
 *
 * This file is part of the 2d library.
 *
 * $Log: fl8clear.c $
 * Revision 1.3  1993/10/19  09:50:18  kaboom
 * Replaced #include "grd.h" with new headers split from grd.h.
 *
 * Revision 1.2  1993/10/08  01:15:08  kaboom
 * Changed quotes in #include liness to angle brackets for Watcom problem.
 *
 * Revision 1.1  1993/02/16  14:14:00  kaboom
 * Initial revision
 */

#include "cnvdat.h"
#include "flat8.h"
#include "lg.h"
#include <string.h>

/* clear a flat8 canvas. */
void flat8_clear(long color) {

    uchar *p;
    int h;
    int w;
    int row;
    ushort short_val;
    double double_stack, doub_vl;
    uint firstbytes, middoubles, lastbytes, fb, md, lb;
    uchar *dst;
    double *dst_doub;
    uint temp;

    color &= 0x00ff;
    p = grd_bm.bits;
    h = grd_bm.h;
    w = grd_bm.w;
    row = grd_bm.row;

    if (w >= 16) // only do doubles if at least two of them (16 bytes)
    {
        // get a 64 bit version of color in doub_vl
        short_val = (uchar)color | color << 8;
        color = (int)short_val | ((int)short_val) << 16;
        *(int *)(&double_stack) = color;
        *((int *)(&double_stack) + 1) = color;
        doub_vl = double_stack;

        lastbytes = w;
        if (firstbytes = (int)p & 3) // check for boundary problems
            lastbytes -= firstbytes;

        middoubles = lastbytes >> 3;
        lastbytes -= middoubles << 3;
    } else {
        lastbytes = w;
        middoubles = 0;
    }

    fb = firstbytes, md = middoubles, lb = lastbytes;
    while (h--) {
        // MLA - inlined this code
        memset(p, color, w);
        /*{
                     firstbytes = fb,middoubles = md,lastbytes = lb;
                           dst = p;

                           if (middoubles)
                            {
                                   // first get to a 4 byte boundary
                                   while (firstbytes--) *(dst++) = color;
                                   dst_doub = (double *) dst;

                                   // now do doubles
                                   while (middoubles--) *(dst_doub++) = doub_vl;
                                   dst = (uchar *) dst_doub;
                            }

                           // do remaining bytes
                           while (lastbytes--) *(dst++) = color;
        }*/

        p += row;
    }
}
