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
//=================================================================
//
//		System Shock - ©1994-1995 Looking Glass Technologies, Inc.
//
//		FIX_SQRT.c	-	Square root routine for fixed-point
// numbers.  Adapted from 80386 asm.
//
//=================================================================

//--------------------
//  Includes
//--------------------
#include "fix.h"
#include "lg.h"
#include <math.h>
#include <stdio.h>

//-----------------------------------------------------------------
//  Calculate the square root of a fixed-point number.
//-----------------------------------------------------------------
fix fix_sqrt(fix num) {
    // fix	res = long_sqrt(num);

    float f = fix_float(num);
    f = sqrtf(f);

    // Make the number a fix and return it
    return fix_from_float(f);
}

//-----------------------------------------------------------------
//  Calculate the square root of a long number.
//-----------------------------------------------------------------
int long_sqrt(int num) {
    // WH dunno, needed?
    if (num == 0)
        return (0);
    // A bit of error checking.
    if (num < 0) {
        ERROR("long_sqrt of negative number!");
        return (0);
    }

    return (int32_t)sqrt(num);
}
