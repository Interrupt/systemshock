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
 * $Source: r:/prj/lib/src/2d/RCS/fl8lop.c $
 * $Revision: 1.2 $
 * $Author: kevin $
 * $Date: 1994/08/16 12:57:18 $
 *
 * lit full perspective texture mapper.
 * scanline processors.
 *
 */

#include "cnvdat.h"
#include "fl8tmapdv.h"
#include "pertyp.h"
#include "plytyp.h"
#include "scrdat.h"
#include "tmapint.h"

// prototypes
void gri_opaque_lit_per_umap_hscan_scanline(grs_per_info *pi, grs_bitmap *bm);
void gri_opaque_lit_per_umap_vscan_scanline(grs_per_info *pi, grs_bitmap *bm);
void gri_opaque_lit_per_umap_hscan_init(grs_bitmap *bm, grs_per_setup *ps);
void gri_opaque_lit_per_umap_vscan_init(grs_bitmap *bm, grs_per_setup *ps);

void opaque_lit_per_hscan_Loop_C(int dx, fix l_du, fix l_dv, fix *pl_u, fix *pl_v, uchar **pp, fix *pl_y_fix,
                                 int *py_cint, fix l_di, fix *pl_i, int l_u_mask, int l_v_shift, int l_v_mask,
                                 fix l_scan_slope, uchar *bm_bits, int gr_row, uchar *ltab) {
    fix l_u, l_v, l_y_fix, l_i;
    int k, y_cint;
    uchar *p;

    l_u = *pl_u;
    l_v = *pl_v;
    l_y_fix = *pl_y_fix;
    l_i = *pl_i;
    p = *pp;
    y_cint = *py_cint;

    for (; dx > 0; dx--) {
        k = ((l_u >> 16) & l_u_mask) + ((l_v >> l_v_shift) & l_v_mask);
        k = bm_bits[k];
        *(p++) = ltab[(fix_light(l_i)) + k];

        k = y_cint;
        y_cint = fix_int(l_y_fix += l_scan_slope);
        if (k != y_cint)
            p += gr_row;

        l_u += l_du;
        l_v += l_dv;
        l_i += l_di;
    }

    *pl_u = l_u;
    *pl_v = l_v;
    *pl_y_fix = l_y_fix;
    *pl_i = l_i;
    *pp = p;
    *py_cint = y_cint;
}

void gri_opaque_lit_per_umap_hscan_scanline(grs_per_info *pi, grs_bitmap *bm) {
    uchar *ltab = grd_screen->ltab;
    int l_x, y_cint, l_u_mask, l_v_mask, l_v_shift;
    fix k, l_u, l_v, l_du, l_dv, l_i, l_di, l_y_fix, l_scan_slope;
    int gr_row;
    uchar *bm_bits, *p;

    // locals used to speed PPC code
    fix test, l_dtl, l_dxl, l_dyl, l_dtr, l_dyr;
    int l_xl, l_xr, l_xr0;

    gr_row = grd_bm.row;
    bm_bits = bm->bits;
    l_xr0 = pi->xr0;
    l_x = pi->x;
    l_di = pi->di;
    l_i = pi->i;
    l_dyr = pi->dyr;
    l_dtr = pi->dtr;
    l_dyl = pi->dyl;
    l_dxl = pi->dxl;
    l_dtl = pi->dtl;
    l_scan_slope = pi->scan_slope;
    l_y_fix = pi->y_fix;
    l_v_shift = pi->v_shift;
    l_v_mask = pi->v_mask;
    l_u_mask = pi->u_mask;
    l_xl = pi->xl;
    l_xr = pi->xr;
    l_u = pi->u;
    l_v = pi->v;
    l_du = pi->du;
    l_dv = pi->dv;

    l_y_fix = l_x * l_scan_slope + fix_make(pi->yp, 0xffff);

    if (l_scan_slope < 0)
        gr_row = -gr_row;

#if InvDiv
    k = fix_div(fix_make(1, 0), pi->denom);
    l_u = pi->u0 + fix_mul_asm_safe(pi->unum, k);
    l_v = pi->v0 + fix_mul_asm_safe(pi->vnum, k);
    l_du = fix_mul_asm_safe(pi->dunum, k);
    l_dv = fix_mul_asm_safe(pi->dvnum, k);
#else
    l_u = pi->u0 + fix_div(pi->unum, pi->denom);
    l_v = pi->v0 + fix_div(pi->vnum, pi->denom);
    l_du = fix_div(pi->dunum, pi->denom);
    l_dv = fix_div(pi->dvnum, pi->denom);
#endif

    l_u += l_x * l_du;
    l_v += l_x * l_dv;

    y_cint = fix_int(l_y_fix);
    p = grd_bm.bits + l_x + y_cint * grd_bm.row;
    if (l_x < l_xl) {
        test = l_x * l_dyl - y_cint * l_dxl + pi->cl;
        for (; l_x < l_xl; l_x++) {
            if (test <= 0) {
                k = (l_u >> 16) & l_u_mask;
                k += (l_v >> l_v_shift) & l_v_mask;
                k = bm_bits[k];
                *p = ltab[(fix_light(l_i)) + k]; // gr_fill_upixel(ltab[(fix_int(l_i)<<8)+k],l_x,y_cint);
            }

            k = y_cint;
            y_cint = fix_int(l_y_fix += l_scan_slope);
            if (k != y_cint) {
                p += gr_row;
                test += l_dtl;
            } else
                test += l_dyl;

            p++;
            l_u += l_du;
            l_v += l_dv;
            l_i += l_di;
        }
    }

    if (l_x < l_xr0) {
        opaque_lit_per_hscan_Loop_C(l_xr0 - l_x, l_du, l_dv, &l_u, &l_v, &p, &l_y_fix, &y_cint, l_di, &l_i, l_u_mask,
                                    l_v_shift, l_v_mask, l_scan_slope, bm_bits, gr_row, ltab);
        l_x = l_xr0;
    }

    if (l_x < l_xr) {
        test = l_x * l_dyr - y_cint * pi->dxr + pi->cr;
        p = grd_bm.bits + l_x + y_cint * grd_bm.row;
        for (; l_x < l_xr; l_x++) {
            if (test >= 0) {
                k = (l_u >> 16) & l_u_mask;
                k += (l_v >> l_v_shift) & l_v_mask;
                k = bm_bits[k];
                *p = ltab[(fix_light(l_i)) + k];
                // gr_fill_upixel(ltab[(fix_int(l_i)<<8)+k],l_x,y_cint);
            }

            k = y_cint;
            y_cint = fix_int(l_y_fix += l_scan_slope);
            if (k != y_cint) {
                p += gr_row;
                test += l_dtr;
            } else
                test += l_dyr;

            p++;
            l_u += l_du;
            l_v += l_dv;
            l_i += l_di;
        }
    }

    pi->y_fix = l_y_fix;
    pi->x = l_x;
    pi->u = l_u;
    pi->v = l_v;
    pi->du = l_du;
    pi->dv = l_dv;
    pi->di = l_di;
    pi->i = l_i;
}

void opaque_lit_per_vscan_Loop_C(int dy, fix l_du, fix l_dv, fix *pl_u, fix *pl_v, uchar **pp, fix *pl_x_fix,
                                 int *px_cint, fix l_di, fix *pl_i, int l_u_mask, int l_v_shift, int l_v_mask,
                                 fix l_scan_slope, uchar *bm_bits, int gr_row, uchar *ltab) {
    fix l_u, l_v, l_x_fix, l_i;
    int k, x_cint;
    uchar *p;

    l_u = *pl_u;
    l_v = *pl_v;
    l_x_fix = *pl_x_fix;
    l_i = *pl_i;
    p = *pp;
    x_cint = *px_cint;

    for (; dy > 0; dy--) {
        k = (l_u >> 16) & l_u_mask;
        k += (l_v >> l_v_shift) & l_v_mask;
        k = bm_bits[k];
        *p = ltab[(fix_light(l_i)) + k];

        k = x_cint;
        x_cint = fix_int(l_x_fix += l_scan_slope);
        if (k != x_cint)
            p -= (k - x_cint);

        p += gr_row;
        l_u += l_du;
        l_v += l_dv;
        l_i += l_di;
    }

    *pl_u = l_u;
    *pl_v = l_v;
    *pl_x_fix = l_x_fix;
    *pl_i = l_i;
    *pp = p;
    *px_cint = x_cint;
}

void gri_opaque_lit_per_umap_vscan_scanline(grs_per_info *pi, grs_bitmap *bm) {
    int k, x_cint;
    uchar *ltab = grd_screen->ltab;

    // locals used to speed PPC code
    fix l_dxr, l_x_fix, l_u, l_v, l_du, l_dv, l_scan_slope, l_dtl, l_dxl, l_dyl, l_dtr, l_dyr, l_i, l_di;
    int l_yl, l_yr0, l_yr, l_y, l_u_mask, l_v_mask, l_v_shift;
    int gr_row, temp_x;
    uchar *bm_bits;
    uchar *p;

    gr_row = grd_bm.row;
    bm_bits = bm->bits;
    l_di = pi->di;
    l_i = pi->i;
    l_dxr = pi->dxr;
    l_x_fix = pi->x_fix;
    l_y = pi->y;
    l_yr = pi->yr;
    l_yr0 = pi->yr0;
    l_yl = pi->yl;
    l_dyr = pi->dyr;
    l_dtr = pi->dtr;
    l_dyl = pi->dyl;
    l_dxl = pi->dxl;
    l_dtl = pi->dtl;
    l_scan_slope = pi->scan_slope;
    l_v_shift = pi->v_shift;
    l_v_mask = pi->v_mask;
    l_u_mask = pi->u_mask;
    l_u = pi->u;
    l_v = pi->v;
    l_du = pi->du;
    l_dv = pi->dv;

    l_x_fix = l_y * l_scan_slope + fix_make(pi->xp, 0xffff);

#if InvDiv
    k = fix_div(fix_make(1, 0), pi->denom);
    l_u = pi->u0 + fix_mul_asm_safe(pi->unum, k);
    l_v = pi->v0 + fix_mul_asm_safe(pi->vnum, k);
    l_du = fix_mul_asm_safe(pi->dunum, k);
    l_dv = fix_mul_asm_safe(pi->dvnum, k);
#else
    l_u = pi->u0 + fix_div(pi->unum, pi->denom);
    l_v = pi->v0 + fix_div(pi->vnum, pi->denom);
    l_du = fix_div(pi->dunum, pi->denom);
    l_dv = fix_div(pi->dvnum, pi->denom);
#endif
    l_u += l_y * l_du;
    l_v += l_y * l_dv;

    x_cint = fix_int(l_x_fix);
    p = grd_bm.bits + x_cint + l_y * gr_row;
    if (l_y < l_yl) {
        fix test = l_y * l_dxl - x_cint * l_dyl + pi->cl;
        for (; l_y < l_yl; l_y++) {
            if (test <= 0) {
                int k = (l_u >> 16) & l_u_mask;
                k += (l_v >> l_v_shift) & l_v_mask;
                k = bm_bits[k];
                *p = ltab[(fix_light(l_i)) + k]; // gr_fill_upixel(ltab[(fix_int(l_i)<<8)+k],x_cint,l_y);
            }

            temp_x = x_cint;
            x_cint = fix_int(l_x_fix += l_scan_slope);
            if (temp_x != x_cint) {
                test += l_dtl;
                p -= temp_x - x_cint;
            } else {
                test += l_dxl;
                x_cint = fix_int(l_x_fix);
            }

            p += gr_row;
            l_u += l_du;
            l_v += l_dv;
            l_i += l_di;
        }
    }

    if (l_y < l_yr0) {
        opaque_lit_per_vscan_Loop_C(l_yr0 - l_y, l_du, l_dv, &l_u, &l_v, &p, &l_x_fix, &x_cint, l_di, &l_i, l_u_mask,
                                    l_v_shift, l_v_mask, l_scan_slope, bm_bits, gr_row, ltab);
        l_y = l_yr0;
    }

    if (l_y < l_yr) {
        fix test = l_y * l_dxr - x_cint * l_dyr + pi->cr;
        p = grd_bm.bits + x_cint + l_y * gr_row;
        for (; l_y < l_yr; l_y++) {
            if (test >= 0) {
                int k = (l_u >> 16) & l_u_mask;
                k += (l_v >> l_v_shift) & l_v_mask;
                k = bm_bits[k];
                *p = ltab[(fix_light(l_i)) + k]; // gr_fill_upixel(ltab[(fix_int(l_i)<<8)+k],x_cint,l_y);
            }

            temp_x = x_cint;
            x_cint = fix_int(l_x_fix += l_scan_slope);
            if (temp_x != x_cint) {
                test += l_dtr;
                p -= temp_x - x_cint;
            } else {
                test += l_dxr;
                x_cint = fix_int(l_x_fix);
            }
            p += gr_row;
            l_u += l_du;
            l_v += l_dv;
            l_i += l_di;
        }
    }

    pi->x_fix = l_x_fix;
    pi->y = l_y;
    pi->u = l_u;
    pi->v = l_v;
    pi->du = l_du;
    pi->dv = l_dv;
    pi->di = l_di;
    pi->i = l_i;
}

extern void gri_lit_per_umap_hscan(grs_bitmap *bm, int n, grs_vertex **vpl, grs_per_setup *ps);
extern void gri_lit_per_umap_vscan(grs_bitmap *bm, int n, grs_vertex **vpl, grs_per_setup *ps);

void gri_opaque_lit_per_umap_hscan_init(grs_bitmap *bm, grs_per_setup *ps) {
    ps->shell_func = (void (*)())gri_lit_per_umap_hscan;
    ps->scanline_func = (void (*)())gri_opaque_lit_per_umap_hscan_scanline;
}

void gri_opaque_lit_per_umap_vscan_init(grs_bitmap *bm, grs_per_setup *ps) {
    ps->shell_func = (void (*)())gri_lit_per_umap_vscan;
    ps->scanline_func = (void (*)())gri_opaque_lit_per_umap_vscan_scanline;
}
