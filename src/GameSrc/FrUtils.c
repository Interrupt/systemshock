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
 * FrUtils.c
 *
 *  MLA - 4/14/95
 *
 *  Contiains Mac specific utils for the renderer (fast draw slot view, full screen, etc.)
 *
 */

#include "FrUtils.h"
#include "gr2ss.h"
#include "Shock.h"
#include "2d.h"

// ------------------
//  GLOBALS
// ------------------
//Handle gDoubleSizeOffHdl = NULL;
grs_canvas gDoubleSizeOffCanvas;

//---------------------------------------------------------------------
//  Allocate the intermediate offscreen buffer for low-res mode in Shock.
//---------------------------------------------------------------------
/*
int AllocDoubleBuffer(int w, int h) {
#if 1
    STUB_ONCE("");
    return 0;
#else
    Size dummy;

    FreeDoubleBuffer(); // If one's there, free it first.

    if (h == 259) // Major Hack!!!  In slot view, the double
        h++;      // buffer needs to be 260 (even number).

    MaxMem(&dummy); // Compact heap before big alloc.

    gDoubleSizeOffHdl = NewHandle(w * h); // Allocate new buffer.
    if (gDoubleSizeOffHdl)                // If successful, make a canvas for it.
    {
        HLockHi(gDoubleSizeOffHdl);
        gr_init_canvas(&gDoubleSizeOffCanvas, (uchar *)*gDoubleSizeOffHdl, BMT_FLAT8, w, h);
        return 1;
    } else
        return 0;
#endif
}
*/
//---------------------------------------------------------------------
//---------------------------------------------------------------------
/*
void FreeDoubleBuffer(void) {
    if (gDoubleSizeOffHdl) // If there's a buffer,
    {
        HUnlock(gDoubleSizeOffHdl);
        DisposeHandle(gDoubleSizeOffHdl); // free it.
        gDoubleSizeOffHdl = NULL;
    }
}
*/
// hard coded to copy from 56,57 to 56+536,57+259 to the screen
#define kFastSlotWide 536
#define kFastSlotHigh 259
#define kFastSlotLeft 28
#define kFastSlotTop  24

#define kFastSlotWide_Half 268
#define kFastSlotHigh_Half 129

#define LoadStoreTwoDoub(a, b) \
    doub1 = src[a];            \
    doub2 = src[b];            \
    dest[a] = doub1;           \
    dest[b] = doub2;

// copy the slot view from offscreen to on
void Fast_Slot_Copy(grs_bitmap *bm) {
    gr_bitmap(bm, SCONV_X(kFastSlotLeft), SCONV_Y(kFastSlotTop));
}

// copy the full screen view from offscreen to on
// hard coded to copy from 0,0 to 640,480 to the screen
void Fast_FullScreen_Copy(grs_bitmap *bm) { gr_bitmap(bm, 0, 0); }

//=================================================================
// Doubling routines
extern bool SkipLines;

// copy the slot view from offscreen to on, doubling it
// extern "C"
//{
//extern void BlitLargeAlign(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus);
//extern void BlitLargeAlignSkip(uchar *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus);
//}

/*
void Fast_Slot_Double(grs_bitmap *bm, long w, long h) {
    if (!SkipLines)
        BlitLargeAlign(bm->bits, gScreenRowbytes, gScreenAddress + (kFastSlotTop * gScreenRowbytes) + kFastSlotLeft, w,
                       h, bm->row);
    else
        BlitLargeAlignSkip(bm->bits, gScreenRowbytes, gScreenAddress + (kFastSlotTop * gScreenRowbytes) + kFastSlotLeft,
                           w, h, bm->row);
}
*/

void FastSlotDouble2Canvas(grs_bitmap *bm, grs_canvas *destCanvas, long w, long h) {
    if (SkipLines) {
        gr_clear(0xFF);
        // BlitLargeAlignSkip(bm->bits, destCanvas->bm.row, destCanvas->bm.bits, w, h + 1, bm->row);
    } else {
        // BlitLargeAlign(bm->bits, destCanvas->bm.row, destCanvas->bm.bits, w, h + 1, bm->row);
    }
}

// copy the full screen view from offscreen to on
// hard coded to copy from 0,0 to 640,480 to the screen
/*
void Fast_FullScreen_Double(grs_bitmap *bm, long w, long h) {
    if (!SkipLines)
        BlitLargeAlign(bm->bits, gScreenRowbytes, gScreenAddress, w, h, bm->row);
    else
        BlitLargeAlignSkip(bm->bits, gScreenRowbytes, gScreenAddress, w, h, bm->row);
}
*/

void FastFullscreenDouble2Canvas(grs_bitmap *bm, grs_canvas *destCanvas, long w, long h) {
    if (SkipLines) {
        gr_clear(0xFF);
        // BlitLargeAlignSkip(bm->bits, destCanvas->bm.row, destCanvas->bm.bits, w, h + 1, bm->row);
    } else {
        // BlitLargeAlign(bm->bits, destCanvas->bm.row, destCanvas->bm.bits, w, h + 1, bm->row);
    }
}
