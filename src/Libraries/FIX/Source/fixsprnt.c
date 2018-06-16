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
** fixsprnt.c	-	sprintf() routines for fixed-point numbers.
**						you may want to use %f and %F in
*lg_sprintf() instead!
**
** $Header: n:/project/lib/src/fix/RCS/fixsprnt.c 1.1 1993/11/04 11:06:24 rex
*Exp $
** $Log: fixsprnt.c $
* Revision 1.1  1993/11/04  11:06:24  rex
* Initial revision
*
*/

#include <stdio.h>
#include <stdlib.h>

#include "fix.h"

//////////////////////////////
//
// Prints out x nicely into str
// and now returns the string too
//
char *fix_sprint(char *str, fix x) {
  ulong tmp;
  uchar neg = FALSE;

  if (x < 0) {
    x = -x;
    neg = TRUE;
  }

  tmp = x & 0xffff;
  tmp *= 10000;
  tmp /= 0xffff;

  if (!neg)
    sprintf(str, "%d.%04lu", x >> 16, tmp);
  else
    sprintf(str, "-%d.%04lu", x >> 16, tmp);

  return str;
}

char *fix24_sprint(char *str, fix24 x) {
  ulong tmp;
  uchar neg = false;

  if (x < 0) {
    x = -x;
    neg = true;
  }

  tmp = x & 0xff;
  tmp *= 1000;
  tmp /= 0xff;

  if (!neg)
    sprintf(str, "%ld.%03lu", x >> 8, tmp);
  else
    sprintf(str, "-%ld.%03lu", x >> 8, tmp);

  return str;
}
