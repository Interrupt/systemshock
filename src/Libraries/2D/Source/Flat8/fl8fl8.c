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
 * $Source: n:/project/lib/src/2d/RCS/fl8fl8.c $
 * $Revision: 1.5 $
 * $Author: kaboom $
 * $Date: 1993/10/19 09:50:21 $
 *
 * Routines for drawing flat 8 bitmaps into a flat 8 canvas.
 *
 * This file is part of the 2d library.
 *
 * $Log: fl8fl8.c $
 * Revision 1.5  1993/10/19  09:50:21  kaboom
 * Replaced #include <grd.h" with new headers split from grd.h.
 *
 * Revision 1.4  1993/10/08  01:15:13  kaboom
 * Changed quotes in #include liness to angle brackets for Watcom problem.
 *
 * Revision 1.3  1993/07/12  23:30:56  kaboom
 * Inline memmove() uses movs.
 *
 * Revision 1.2  1993/03/29  18:22:11  kaboom
 * Changed to inline version of memmove.
 *
 * Revision 1.1  1993/02/16  14:14:16  kaboom
 * Initial revision
 *
 ********************************************************************
 * Log from old flat8.c:
 * Revision 1.7  1992/12/14  18:08:40  kaboom
 * Added handing for transparency in the flat8_flat8_ubitmap()
 * routine.
 */

#include "bitmap.h"
#include "cnvdat.h"
#include "flat8.h"
#include "lg.h"
#include <string.h>

void flat8_flat8_ubitmap(grs_bitmap *bm, short x, short y) {
    uchar *m_src;
    uchar *m_dst;
    int w = bm->w;
    int h = bm->h;
    int i;
    int brow, grow;

    brow = bm->row;
    grow = grd_bm.row;

    m_src = bm->bits;
    m_dst = grd_bm.bits + grow * y + x;

    if (bm->flags & BMF_TRANS)
        while (h--) {
            for (i = 0; i < w; i++)
                if (m_src[i] != 0)
                    m_dst[i] = m_src[i];
            m_src += brow;
            m_dst += grow;
        }
    else
        while (h--) {
            memmove(m_dst, m_src, w);

            m_src += brow;
            m_dst += grow;
        }
}
