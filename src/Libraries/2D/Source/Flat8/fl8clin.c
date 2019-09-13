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
 * $Source: r:/prj/lib/src/2d/RCS/fl8clin.c $
 * $Revision: 1.7 $
 * $Author: kevin $
 * $Date: 1994/10/17 14:59:57 $
 *
 * Routine to draw an rgb shaded line to a flat 8 canvas.
 *
 * This file is part of the 2d library.
 *
 * $Log: fl8clin.c $
 * Revision 1.7  1994/10/17  14:59:57  kevin
 * Use palette macros in preparation for switch to palette globals.
 *
 * Revision 1.6  1994/06/11  01:24:08  lmfeeney
 * guts of the routine moved to fl8{c,s}lin.h, per fill type
 * line drawers are created by defining macros and including
 * this file
 *
 * Revision 1.5  1994/05/06  18:18:38  lmfeeney
 * rewritten for greater accuracy and speed
 *
 * Revision 1.4  1994/05/01  05:34:38  lmfeeney
 * rewritten using simple dda algorithm (+ bit twiddle hack) for greater
 * speed (20/30%) and improvement for e.g. diagonal lines
 *
 * Revision 1.3  1993/10/19  09:50:19  kaboom
 * Replaced #include <grd.h" with new headers split from grd.h.
 *
 * Revision 1.2  1993/10/01  15:43:46  kaboom
 * Pared down include files to reduce dependencies.
 *
 * Revision 1.1  1993/07/01  22:11:55  spaz
 * Initial revision
 */

#include <stdint.h>

#include "cnvdat.h"
#include "fix.h"
#include "plytyp.h"
#include "scrdat.h"

// MLA #pragma off (unreferenced)

#define fix_make_nof(x) fix_make(x, 0x0000)
#define macro_get_ipal(r, g, b) (int32_t)((r >> 19) & 0x1f) | ((g >> 14) & 0x3e0) | ((b >> 9) & 0x7c00)

#undef macro_plot_rgb
#define macro_plot_rgb(x, p, i) \
    do {                        \
        p[x] = grd_ipal[i];     \
    } while (0)

void gri_flat8_ucline_norm(int32_t c, int32_t parm, grs_vertex *v0, grs_vertex *v1) {
    fix x0, y0, x1, y1;
    fix dx, dy; /* deltas in x and y */
    fix t;      /* tmp */

    uchar r0, g0, b0, r1, g1, b1; /* rgb values of endpt colors */
    fix r, g, b;                  /* current intensities */
    fix dr, dg, db;               /* deltas for each of rgb */
    int32_t i;                    /* color index */

    uchar *p; /* ptr into canvas */

    x0 = v0->x;
    y0 = v0->y;
    x1 = v1->x;
    y1 = v1->y;

    r0 = (uchar)(v0->u);
    g0 = (uchar)(v0->v);
    b0 = (uchar)(v0->w);
    r1 = (uchar)(v1->u);
    g1 = (uchar)(v1->v);
    b1 = (uchar)(v1->w);

    /* set endpoints
       note that this cannot go negative or change octant, since the ==
       case is excluded */

    if (x0 < x1) {
        x1 -= 1; /* e.g. - epsilon */
    } else if (x0 > x1) {
        x0 -= 1;
    }

    if (y0 < y1) {
        y1 -= 1;
    } else if (y0 > y1) {
        y0 -= 1;
    }

    dx = fix_trunc(x1) - fix_trunc(x0); /* x extent in pixels, (macro is flakey) */
    dx = fix_abs(dx);
    dy = fix_trunc(y1) - fix_trunc(y0); /* y extent in pixels */
    dy = fix_abs(dy);

    if (dx == 0 && dy == 0)
        return;

    /* three cases: absolute value dx < = > dy

       along the longer dimension, the fixpoint x0 (or y0) is treated
       as an int

       the points are swapped if needed and the rgb initial and deltas
       are calculated accordingly

       there are two or three sub-cases - a horizontal or vertical line,
       and the dx or dy being added or subtracted.  dx and dy
       are kept as absolute values and +/- is managed in
       two separate inner loops if it is a y change, since you need to
       manage the canvas pointer

       if y is being changed by 'dy' and x is being incremented, do a
       FunkyBitCheck (TM) to see whether the integer part of y has changed
       and if it has, resetting the the canvas pointer to the next row

       if x is being changed by 'dx' and y is being incremented, just
       add or subtract row to increment y in the canvas

       the endpoints are walked inclusively in all cases, see above

       45' degree lines are explicitly special cased -- because it
       all runs as integers, but it's probably not frequent enough
       to justify the check

     */

    if (dx > dy) {

        x0 = fix_int(x0);
        x1 = fix_int(x1);

        if (x0 < x1) {
            r = fix_make(r0, 0);
            g = fix_make(g0, 0);
            b = fix_make(b0, 0);
            dr = fix_div(fix_make_nof(r1 - r0), dx);
            dg = fix_div(fix_make_nof(g1 - g0), dx);
            db = fix_div(fix_make_nof(b1 - b0), dx);

            p = grd_bm.bits + grd_bm.row * (fix_int(y0)); /* set canvas ptr */

        } else {
            t = x0;
            x0 = x1;
            x1 = t;
            t = y0;
            y0 = y1;
            y1 = t;

            r = fix_make(r1, 0);
            g = fix_make(g1, 0);
            b = fix_make(b1, 0);
            dr = fix_div(fix_make_nof(r0 - r1), dx);
            dg = fix_div(fix_make_nof(g0 - g1), dx);
            db = fix_div(fix_make_nof(b0 - b1), dx);

            p = grd_bm.bits + grd_bm.row * (fix_int(y0));
        }

        if ((fix_int(y0)) == (fix_int(y1))) {
            while (x0 <= x1) {
                i = macro_get_ipal(r, g, b);
                macro_plot_rgb(x0, p, i);
                x0++;
                r += dr;
                g += dg;
                b += db;
            }
        } else if (y0 < y1) {
            dy = fix_div((y1 - y0), dx);
            while (x0 <= x1) {
                i = macro_get_ipal(r, g, b);
                macro_plot_rgb(x0, p, i);
                x0++;
                y0 += dy;
                p += (grd_bm.row & (-(fix_frac(y0) < dy)));
                r += dr;
                g += dg;
                b += db;
            }
        } else {
            dy = fix_div((y0 - y1), dx);
            while (x0 <= x1) {
                i = macro_get_ipal(r, g, b);
                macro_plot_rgb(x0, p, i);
                x0++;
                p -= (grd_bm.row & (-(fix_frac(y0) < dy)));
                y0 -= dy;
                r += dr;
                g += dg;
                b += db;
            }
        }
    }

    else if (dy > dx) {

        y0 = fix_int(y0);
        y1 = fix_int(y1);

        if (y0 < y1) {
            r = fix_make(r0, 0);
            g = fix_make(g0, 0);
            b = fix_make(b0, 0);
            dr = fix_div(fix_make_nof(r1 - r0), dy);
            dg = fix_div(fix_make_nof(g1 - g0), dy);
            db = fix_div(fix_make_nof(b1 - b0), dy);

            p = grd_bm.bits + grd_bm.row * y0;

        } else {
            t = x0;
            x0 = x1;
            x1 = t;
            t = y0;
            y0 = y1;
            y1 = t;

            r = fix_make(r1, 0);
            g = fix_make(g1, 0);
            b = fix_make(b1, 0);
            dr = fix_div(fix_make_nof(r0 - r1), dy);
            dg = fix_div(fix_make_nof(g0 - g1), dy);
            db = fix_div(fix_make_nof(b0 - b1), dy);

            p = grd_bm.bits + grd_bm.row * y0;
        }

        if ((fix_int(x0)) == (fix_int(x1))) {
            x0 = fix_int(x0);
            while (y0 <= y1) {
                i = macro_get_ipal(r, g, b);
                macro_plot_rgb(x0, p, i);
                y0++;
                p += grd_bm.row;
                r += dr;
                g += dg;
                b += db;
            }
        } else {
            dx = fix_div((x1 - x0), dy);
            while (y0 <= y1) {
                i = macro_get_ipal(r, g, b);
                macro_plot_rgb(fix_fint(x0), p, i);
                x0 += dx;
                y0++;
                p += grd_bm.row;
                r += dr;
                g += dg;
                b += db;
            }
        }
    } else { /* dy == dx, walk the x axis, all integers */

        x0 = fix_int(x0);
        x1 = fix_int(x1);
        y0 = fix_int(y0);
        y1 = fix_int(y1);

        if (x0 < x1) {
            r = fix_make(r0, 0);
            g = fix_make(g0, 0);
            b = fix_make(b0, 0);
            dr = fix_div(fix_make_nof(r1 - r0), dx);
            dg = fix_div(fix_make_nof(g1 - g0), dx);
            db = fix_div(fix_make_nof(b1 - b0), dx);

            p = grd_bm.bits + grd_bm.row * y0; /* set canvas ptr */

        } else {
            t = x0;
            x0 = x1;
            x1 = t;
            t = y0;
            y0 = y1;
            y1 = t;

            r = fix_make(r1, 0);
            g = fix_make(g1, 0);
            b = fix_make(b1, 0);
            dr = fix_div(fix_make_nof(r0 - r1), dx);
            dg = fix_div(fix_make_nof(g0 - g1), dx);
            db = fix_div(fix_make_nof(b0 - b1), dx);

            p = grd_bm.bits + grd_bm.row * y0;
        }

        if (y0 < y1) {
            while (y0 <= y1) {
                i = macro_get_ipal(r, g, b);
                macro_plot_rgb(x0, p, i);
                x0++;
                y0++;
                p += grd_bm.row;
                r += dr;
                g += dg;
                b += db;
            }
        } else {
            while (y0 >= y1) {
                i = macro_get_ipal(r, g, b);
                macro_plot_rgb(x0, p, i);
                x0++;
                y0--;
                p -= grd_bm.row;
                r += dr;
                g += dg;
                b += db;
            }
        }
    }
}

/*
#undef macro_plot_rgb
#define macro_plot_rgb(x, p, i)                        \
    do {                                               \
        p[x] = (long)(((uchar *)parm)[(grd_ipal[i])]); \
    } while (0)

void gri_flat8_ucline_clut(long c, long parm, grs_vertex *v0, grs_vertex *v1) {
#include "fl8clin.h"
}

#undef macro_plot_rgb
#define macro_plot_rgb(x, p, i)      \
    do {                             \
        p[x] = p[x] ^ (grd_ipal[i]); \
    } while (0)

void gri_flat8_ucline_xor(long c, long parm, grs_vertex *v0, grs_vertex *v1) {
#include "fl8clin.h"
}
*/

/* punt */
/*
#undef macro_plot_rgb
#define macro_plot_rgb(x, p, i)                        \
    do {                                               \
        p[x] = (long)(((uchar *)parm)[(grd_ipal[i])]); \
    } while (0)

void gri_flat8_ucline_blend(long c, long parm, grs_vertex *v0, grs_vertex *v1){
#include "fl8clin.h"
}
*/