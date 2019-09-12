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
 * $Header: r:/prj/lib/src/h/RCS/2dres.h 1.6 1994/07/06 18:38:48 jaemz Exp $
 *
 * Macros to use 2d calls with resources
 *
 * $Log: 2dres.h $
 * Revision 1.6  1994/07/06  18:38:48  jaemz
 * Added fields to cylbm frame
 *
 * Revision 1.5  1994/06/15  11:51:49  jaemz
 * Added cylindrical bitmap object data types
 *
 * Revision 1.4  1994/01/27  13:21:19  eric
 * Added gr_cpy_pal_image to copy image pallette from
 * REFerence to a pallette in memory.
 *
 * Revision 1.3  1993/09/28  01:12:21  kaboom
 * Converted #include "xxx" to #include <xxx> for watcom.
 *
 * Revision 1.2  1993/08/24  20:04:52  rex
 * Turned framedesc's updateArea into union of updateArea, anchorArea, anchorPt
 *
 * Revision 1.1  1993/04/27  12:06:52  rex
 * Initial revision
 */

#ifndef _2DRES_H
#define _2DRES_H

#include "2d.h"
#include "res.h"
#include "rect.h"

#pragma pack(push, 2)

// A Ref in a resource gets you a Frame Descriptor:

typedef struct {
    grs_bitmap bm; // embedded bitmap, bm.bits set to NULL
    union {
        LGRect updateArea; // update area (for anims)
        LGRect anchorArea; // area to anchor sub-bitmap
        LGPoint anchorPt;  // point to anchor from
    };
    int32_t pallOff; // offset to pallette
                     // bitmap's bits follow immediately
} FrameDesc;
#pragma pack(pop)

#endif
