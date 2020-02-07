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
 * $Source: n:/project/lib/src/2d/RCS/str.h $
 * $Revision: 1.5 $
 * $Author: lmfeeney $
 * $Date: 1994/04/09 00:31:21 $
 *
 * Prototypes for non-table string functions.
 *
 * This file is part of the 2d library.
 *
 * $Log: str.h $
 * Revision 1.5  1994/04/09  00:31:21  lmfeeney
 * added new height and wrapping routines, added #defines
 * for compatibility with string and char routines taking
 * grs_font * as first arg
 *
 * Revision 1.4  1993/06/02  21:29:21  kaboom
 * Moved n argument for gr_string_nwidth from last to second.
 *
 * Revision 1.3  1993/06/02  16:33:42  kaboom
 * Added prototypes for various new size & n-char routines.
 *
 * Revision 1.2  1993/04/08  18:57:12  kaboom
 * Added prototypes for character functions.
 *
 * Revision 1.1  1993/04/08  16:28:06  kaboom
 * Initial revision
 */

/* prototypes for non-table driven string handling routines. */
extern void gr_font_string_size(grs_font *font, char *string, short *width, short *height);
extern short gr_font_string_width(grs_font *font, char *string);
extern short gr_font_char_width(grs_font *f, char c);
extern void gr_font_char_size(grs_font *font, char c, short *width, short *height);
extern int gr_font_string_wrap(grs_font *pfont, char *ps, short width);
extern void gr_font_string_unwrap(char *s);
