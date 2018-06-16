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
#include <math.h>

//--------------------
//  Table of square root guesses
//--------------------
ubyte pGuessTable[256] = {
    1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,
    4,  4,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  6,  6,
    6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
    8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  9,
    9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15};


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
//  Calculate the square root of a wide (64-bit) number.
//-----------------------------------------------------------------
int32_t quad_sqrt(int32_t hi, uint32_t lo) {
  //	uchar	testb, trans;
  //	uchar	shift;
  //	long		divisor, temp, savediv, rem;

  // Parameter checking

  if (hi == 0) // If there is no high word
  {
    if (lo > 0) // If lo word is positive, just call long_sqrt()
      return (sqrt(lo));
    if (lo == 0) // If lo word is zero, return 0.
      return (0);
  }
  if (hi < 0) // If a negative number, return 0.
    return (0);

  // If 'hi' is non-zero, call FixMath's WideSquareRoot.
  AWide a;
  a.hi = hi;
  a.lo = lo;
  int64_t x;
  ASSIGN_WIDE_TO_64(x, &a);
  return (unsigned int)sqrtl((long double)x);
}

//-----------------------------------------------------------------
//  Calculate the square root of a long number.
//-----------------------------------------------------------------
int long_sqrt(int num) {
  fix savediv;
  fix temp;
  uchar testb, trans;
  uchar shift;
  fix divisor;
  short i;

  if (num == 0) // A bit of error checking.
    return (0);
  if (num < 0)
    return (0);

  // Find the highest byte that is non-zero, look up a good first guess for
  // that byte, and shift it the appropriate amount.

  testb = (uchar)(num >> 24);
  if (testb != 0) {
    trans = pGuessTable[testb];
    shift = 12;
    goto found_byte;
  }
  testb = (uchar)(num >> 16);
  if (testb != 0) {
    trans = pGuessTable[testb];
    shift = 8;
    goto found_byte;
  }
  testb = (uchar)(num >> 8);
  if (testb != 0) {
    trans = pGuessTable[testb];
    shift = 4;
    goto found_byte;
  }
  testb = (uchar)num;
  if (testb != 0) {
    trans = pGuessTable[testb];
    shift = 0;
  }

  // We now have the good initial guess.  Shift it the appropriate amount.

found_byte:
  divisor = trans;
  divisor = divisor << shift;

  // Experience has shown that we almost always go through the loop
  // just about three times.  To avoid compares and jumps, we iterate
  // three times without even thinking about it, and then start checking
  // to see if our answer is correct.

  for (i = 0; i < 2; i++) {
    temp = num / divisor;
    divisor += temp;
    divisor = divisor >> 1;
  }

  // Starting with the third iteration, we now actually check for a match.

  while (1) {
    temp = num / divisor;
    if (temp == divisor)
      break;

    savediv = divisor;
    divisor += temp;
    divisor = divisor >> 1;
    if (temp == divisor) {
      if (num % savediv != 0)
        divisor++;
      break;
    } else if (savediv == divisor) {
      if (num % savediv != 0)
        divisor++;
      break;
    }
  }
  return (divisor);
}
