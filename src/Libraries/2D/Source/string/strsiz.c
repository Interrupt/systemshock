/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.
Copyright (C) 2018-2020 Shockolate Project

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
 * $Source: n:/project/lib/src/2d/RCS/strsiz.c $
 * $Revision: 1.5 $
 * $Author: lmfeeney $
 * $Date: 1994/06/15 01:17:16 $
 *
 * String width and height calculator.
 *
 * This file is part of the 2d library.
 *
 * $Log: strsiz.c $
 * Revision 1.5  1994/06/15  01:17:16  lmfeeney
 * support extended ascii (c > 127) w\ uchars, don't change fn i\f
 *
 * Revision 1.4  1994/04/09  07:38:20  lmfeeney
 * added grs_font *  as first argument, #define for compatibility in str.h
 *
 * Revision 1.3  1993/10/19  09:57:59  kaboom
 * Replaced #include   new headers.
 *
 * Revision 1.2  1993/10/08  01:16:28  kaboom
 * Changed quotes in #include lines to angle brackets for Watcom.
 *
 * Revision 1.1  1993/06/02  16:24:23  kaboom
 * Initial revision
 */

#include "chr.h"
#include "grs.h"
#include "str.h"

/**
 * Calculate the width and height of a string in the specified font, and return in the given pointers.
 * @param font font
 * @param string string
 * @param width returned width
 * @param height returned height
 */
void gr_font_string_size(grs_font *font, char *string, short *width, short *height) {
    short *offset_tab; /* table of character offsets */
    short offset;      /* offset of current character */
    short w_lin = 0;   /* current line's width so far */
    short w_str = 0;   /* width of widest line */
    short h_str;       /* height of string */
    uchar c;           /* current character */

    offset_tab = font->off_tab;
    h_str = font->h;
    while ((c = (uchar)(*string++)) != '\0') {
        if (c == CHAR_SOFTSP)
            continue;
        if (c == '\n' || c == CHAR_SOFTCR) {
            if (w_lin > w_str)
                w_str = w_lin;
            w_lin = 0;
            h_str += font->h;
            continue;
        }
        offset = offset_tab[c - font->min];
        w_lin += offset_tab[c - font->min + 1] - offset;
    }
    *width = (w_lin > w_str) ? w_lin : w_str;
    *height = h_str;
}

/**
 * Returns the width of string in pixels for the specified font
 * @param font font
 * @param string string
 * @return width of string
 */
short gr_font_string_width(grs_font *font, char *string) {
    short width = 0, height = 0;
    gr_font_string_size(font, string, &width, &height);
    return width;
}
