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
//
// $Source: r:/prj/lib/src/2d/RCS/fl8dbl.asm $
// $Revision: 1.3 $
// $Author: kevin $
// $Date: 1994/09/08 00:00:18 $
//
// Bitmap doubling primitives.
//

#include "blndat.h"
#include "cnvdat.h"
#include "flat8.h"
#include "grs.h"
#include <stdbool.h>
#include <stdlib.h>

// ------------------------------------------------------------------------
// PowerPC routines
// ------------------------------------------------------------------------
// ========================================================================
void flat8_flat8_h_double_ubitmap(grs_bitmap *bm) {
    extern void DebugString(char *msg);
    DebugString("call mark");
    /* 	int		h,v,endh,endv;
            uchar *src=bm->bits, *dst=grd_bm.bits;
            long	srcAdd,dstAdd;
            uchar	temp;

            srcAdd = bm->row-bm->w;
            dstAdd = grd_bm.row - (bm->w<<1);
            endh = bm->w;
            endv = bm->h;

            for (v=0; v<endv; v++)
             {
                    for (h=0; h<endh; h++)
                     {
                            temp = *(src++);
                            *(dst++) = temp;
                            *(dst++) = temp;
                     }

                    src+=srcAdd;
                    dst+=dstAdd;
             }*/
}

// ========================================================================
void flat8_flat8_smooth_h_double_ubitmap(grs_bitmap *srcb, grs_bitmap *dstb) {
    int h, v, endh, endv;
    uchar *src = srcb->bits, *dst = dstb->bits;
    long srcAdd, dstAdd;
    ushort curpix, tempshort;
    uchar *local_grd_half_blend;

    local_grd_half_blend = grd_half_blend;
    if (!local_grd_half_blend)
        return;

    srcAdd = (srcb->row - srcb->w) - 1;
    dstAdd = dstb->row - (srcb->w << 1);
    endh = srcb->w - 1;
    endv = srcb->h;

    for (v = 0; v < endv; v++) {
        curpix = *(short *)src;
        src += 2;
        for (h = 0; h < endh; h++) {
            tempshort = curpix & 0xff00;
            tempshort |= local_grd_half_blend[curpix];
            *(ushort *)dst = tempshort;
            dst += 2;
            curpix = (curpix << 8) | *(src++);
        }

        // double last pixel
        curpix >>= 8;
        *(dst++) = curpix;
        *(dst++) = curpix;

        src += srcAdd;
        dst += dstAdd;
    }
}

// ========================================================================
// src = eax, dest = edx
void flat8_flat8_smooth_hv_double_ubitmap(grs_bitmap *src, grs_bitmap *dst) {
    int tempH, tempW, temp, savetemp;
    uchar *srcPtr, *dstPtr;
    uchar *shvd_read_row1, *shvd_write, *shvd_read_row2, *shvd_read_blend;
    ushort tempc;

    dstPtr = dst->bits;
    srcPtr = src->bits;

    // HAX HAX HAX no smooth doubling for now!
    for (int y = 0; y < src->h; y++) {
        for (int x = 0; x < src->w; x++) {
            *(dstPtr) = *srcPtr;
            *(dstPtr + 1) = *srcPtr;
            *(dstPtr + src->w * 2) = *srcPtr;
            *((dstPtr + src->w * 2) + 1) = *srcPtr;
            dstPtr += 2;
            srcPtr++;
        }
        dstPtr += src->w * 2;
    }

    return;

    dst->row <<= 1;
    flat8_flat8_smooth_h_double_ubitmap(src, dst);

    dst->row = tempW = dst->row >> 1;
    dstPtr = dst->bits;

    tempH = src->h - 1;
    temp = src->w << 1;
    dstPtr += temp;
    temp = -temp;

    shvd_read_row1 = dstPtr;
    dstPtr += tempW;
    shvd_write = dstPtr - 1;
    dstPtr += tempW;
    shvd_read_row2 = dstPtr;
    shvd_read_blend = grd_half_blend;
    savetemp = temp;

    do {
        do {
            tempc = shvd_read_row1[temp];
            tempc |= ((ushort)shvd_read_row2[temp]) << 8;
            temp++;

            shvd_write[temp] = shvd_read_blend[tempc];
        } while (temp != 0);

        if (--tempH == 0)
            break;

        shvd_read_row1 = dstPtr;
        dstPtr += tempW;
        shvd_write = dstPtr - 1;
        dstPtr += tempW;
        shvd_read_row2 = dstPtr;
        temp = savetemp;
    } while (true);

    // do last row
    srcPtr = dstPtr + savetemp;
    dstPtr += tempW + savetemp;
    savetemp = -savetemp;

    for (; savetemp > 0; savetemp--)
        *(dstPtr++) = *(srcPtr++);
}
