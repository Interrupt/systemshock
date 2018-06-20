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
 * $Source: r:/prj/lib/src/2d/RCS/fl8opl.c $
 * $Revision: 1.2 $
 * $Author: kevin $
 * $Date: 1994/08/16 12:57:20 $
 *
 * full perspective texture mapper.
 * scanline processors.
 *
 */

#include "cnvdat.h"
#include "grpix.h"
#include "pertyp.h"
#include "plytyp.h"

// prototypes
void gri_opaque_per_umap_hscan_scanline(grs_per_info *pi, grs_bitmap *bm);
void gri_opaque_per_umap_vscan_scanline(grs_per_info *pi, grs_bitmap *bm);
void gri_opaque_per_umap_hscan_init(grs_bitmap *bm, grs_per_setup *ps);
void gri_opaque_per_umap_vscan_init(grs_bitmap *bm, grs_per_setup *ps);

void gri_opaque_per_umap_hscan_scanline(grs_per_info *pi, grs_bitmap *bm) {

    // make SURE these come out in registers
    register int x, k, y_cint;
    register uchar *p;
    register int gr_row, l_u_mask, l_v_mask, l_v_shift;
    register fix l_du, l_dv, l_y_fix, l_scan_slope, test, l_u, l_v;
    register uchar *bm_bits;
    register fix l_dl, l_dt;

    int l_x;

    gr_row = grd_bm.row;
    bm_bits = bm->bits;
    l_u = pi->u;
    l_v = pi->v;
    l_scan_slope = pi->scan_slope;
    l_v_shift = pi->v_shift;
    l_v_mask = pi->v_mask;
    l_u_mask = pi->u_mask;
    l_du = pi->du;
    l_dv = pi->dv;
    l_y_fix = pi->y_fix;
    l_x = pi->x;

    l_y_fix = l_x * l_scan_slope + fix_make(pi->yp, 0xffff);
    l_u = pi->u0 + fix_div(pi->unum, pi->denom);
    l_v = pi->v0 + fix_div(pi->vnum, pi->denom);
    l_du = fix_div(pi->dunum, pi->denom);
    l_dv = fix_div(pi->dvnum, pi->denom);
    l_u += l_x * l_du;
    l_v += l_x * l_dv;

    y_cint = fix_int(l_y_fix);
    if (l_scan_slope < 0)
        gr_row = -gr_row;
    p = grd_bm.bits + l_x + (y_cint * grd_bm.row);
    if (l_x < pi->xl) {
        l_dl = pi->dyl;
        l_dt = pi->dtl;
        test = l_x * pi->dyl - y_cint * pi->dxl + pi->cl;
        x = pi->xl - l_x;
        l_x = pi->xl;
        for (; x > 0; x--) {
            if (test <= 0) {
                k = (l_u >> 16) & l_u_mask;
                k += (l_v >> l_v_shift) & l_v_mask;
                *p = bm_bits[k]; // gr_fill_upixel(bm_bits[k],l_x,y_cint);
            }
            k = y_cint;
            y_cint = fix_int(l_y_fix += l_scan_slope);

            if (k != y_cint) {
                p += gr_row;
                test += l_dt;
            } else
                test += l_dl;

            p++;
            l_u += l_du;
            l_v += l_dv;
        }
    }

    if (l_x < pi->xr0) {
        x = pi->xr0 - l_x;
        l_x = pi->xr0;
        for (; x > 0; x--) {
            k = (l_u >> 16) & l_u_mask;
            k += (l_v >> l_v_shift) & l_v_mask;
            *(p++) = bm_bits[k]; // gr_fill_upixel(bm_bits[k],l_x,y_cint);
            k = y_cint;
            y_cint = fix_int(l_y_fix += l_scan_slope);

            if (k != y_cint)
                p += gr_row;

            l_u += l_du;
            l_v += l_dv;
        }
    }

    if (l_x < pi->xr) {
        l_dl = pi->dyr;
        test = l_x * l_dl - y_cint * pi->dxr + pi->cr;
        p = grd_bm.bits + l_x + y_cint * grd_bm.row;
        x = pi->xr - l_x;
        l_dt = pi->dtr;
        l_x = pi->xr;
        for (; x > 0; x--) {
            if (test >= 0) {
                k = (l_u >> 16) & l_u_mask;
                k += (l_v >> l_v_shift) & l_v_mask;
                *p = bm_bits[k]; // gr_fill_upixel(bm_bits[k],l_x,y_cint);
            }
            k = y_cint;
            y_cint = fix_int(l_y_fix += l_scan_slope);

            if (k != y_cint) {
                p += gr_row;
                test += l_dt;
            } else
                test += l_dl;

            p++;
            l_u += l_du;
            l_v += l_dv;
        }
    }

    pi->y_fix = l_y_fix;
    pi->x = l_x;
    pi->u = l_u;
    pi->v = l_v;
    pi->du = l_du;
    pi->dv = l_dv;
}

void gri_opaque_per_umap_vscan_scanline(grs_per_info *pi, grs_bitmap *bm) {
    int x_cint;

    // locals used to speed PPC code
    fix l_dxr, l_x_fix, l_u, l_v, l_du, l_dv, l_scan_slope, l_dtl, l_dxl, l_dyl, l_dtr, l_dyr;
    int l_yl, l_yr0, l_yr, l_y, l_u_mask, l_v_mask, l_v_shift;
    int gr_row, temp_x;
    uchar *bm_bits;
    uchar *p;

    gr_row = grd_bm.row;
    bm_bits = bm->bits;
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

    l_u = pi->u0 + fix_div(pi->unum, pi->denom);
    l_v = pi->v0 + fix_div(pi->vnum, pi->denom);
    l_du = fix_div(pi->dunum, pi->denom);
    l_dv = fix_div(pi->dvnum, pi->denom);
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
                *p = bm_bits[k]; // gr_fill_upixel(bm_bits[k],x_cint,l_y);
            }
            temp_x = x_cint;
            x_cint = fix_int(l_x_fix += l_scan_slope);

            if (temp_x != x_cint) {
                test += l_dtl;
                p -= (temp_x - x_cint);
            } else
                test += l_dxl;

            p += gr_row;
            l_u += l_du;
            l_v += l_dv;
        }
    }

    for (; l_y < l_yr0; l_y++) {
        int k = (l_u >> 16) & l_u_mask;
        k += (l_v >> l_v_shift) & l_v_mask;
        *p = bm_bits[k]; // gr_fill_upixel(bm_bits[k],x_cint,l_y);

        temp_x = x_cint;
        x_cint = fix_int(l_x_fix += l_scan_slope);
        if (temp_x != x_cint)
            p -= (temp_x - x_cint);

        p += gr_row;
        l_u += l_du;
        l_v += l_dv;
    }

    if (l_y < l_yr) {
        fix test = l_y * l_dxr - x_cint * l_dyr + pi->cr;
        p = grd_bm.bits + x_cint + l_y * gr_row;
        for (; l_y < l_yr; l_y++) {
            if (test >= 0) {
                int k = (l_u >> 16) & l_u_mask;
                k += (l_v >> l_v_shift) & l_v_mask;
                *p = bm_bits[k]; // gr_fill_upixel(bm_bits[k],x_cint,l_y);
            }

            temp_x = x_cint;
            x_cint = fix_int(l_x_fix += l_scan_slope);
            if (temp_x != x_cint) {
                test += l_dtr;
                p -= (temp_x - x_cint);
            } else
                test += l_dxr;

            p += gr_row;
            l_u += l_du;
            l_v += l_dv;
        }
    }

    pi->x_fix = l_x_fix;
    pi->y = l_y;
    pi->u = l_u;
    pi->v = l_v;
    pi->du = l_du;
    pi->dv = l_dv;
}

extern void gri_per_umap_hscan(grs_bitmap *bm, int n, grs_vertex **vpl, grs_per_setup *ps);
extern void gri_per_umap_vscan(grs_bitmap *bm, int n, grs_vertex **vpl, grs_per_setup *ps);

void gri_opaque_per_umap_hscan_init(grs_bitmap *bm, grs_per_setup *ps) {
    ps->shell_func = (void (*)())gri_per_umap_hscan;
    ps->scanline_func = (void (*)())gri_opaque_per_umap_hscan_scanline;
}

void gri_opaque_per_umap_vscan_init(grs_bitmap *bm, grs_per_setup *ps) {
    ps->shell_func = (void (*)())gri_per_umap_vscan;
    ps->scanline_func = (void (*)())gri_opaque_per_umap_vscan_scanline;
}
