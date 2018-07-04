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
 * $Source: r:/prj/lib/src/2d/RCS/genw.c $
 * $Revision: 1.4 $
 * $Author: kevin $
 * $Date: 1994/08/16 12:50:15 $
 *
 * Routines to floor texture map a flat8 bitmap to a generic canvas.
 *
 * This file is part of the 2d library.
 *
 */

#include "cnvdat.h"
#include "fl8tf.h"
#include "fl8tmapdv.h"
#include "gente.h"
#include "grnull.h"
#include "grpix.h"
#include "poly.h"
#include "stdio.h"
#include "tmapint.h"
#include "vtab.h"

#include <stdbool.h>

int gri_wall_umap_loop(grs_tmap_loop_info *tli);
int gri_wall_umap_loop_1D(grs_tmap_loop_info *tli);

int gri_wall_umap_loop(grs_tmap_loop_info *tli) {
    fix u, v, du, dv, dy, d;

    // locals used to store copies of tli-> stuff, so its in registers on the PPC
    int k, y;
    uchar t_wlog;
    ulong t_mask;
    long *t_vtab;
    uchar *t_bits;
    uchar *p_dest;
    fix inv_dy;
    uchar temp_pix;
    uchar *t_clut;
    long gr_row;

#if InvDiv
    inv_dy = fix_div(fix_make(1, 0), tli->w);
    u = fix_mul_asm_safe(tli->left.u, inv_dy);
    du = fix_mul_asm_safe(tli->right.u, inv_dy) - u;
    v = fix_mul_asm_safe(tli->left.v, inv_dy);
    dv = fix_mul_asm_safe(tli->right.v, inv_dy) - v;
#else
    u = fix_div(tli->left.u, tli->w);
    du = fix_div(tli->right.u, tli->w) - u;
    v = fix_div(tli->left.v, tli->w);
    dv = fix_div(tli->right.v, tli->w) - v;
#endif

    dy = tli->right.y - tli->left.y;

    t_vtab = tli->vtab;
    t_bits = tli->bm.bits;

    t_clut = tli->clut;
    t_mask = tli->mask;
    t_wlog = tli->bm.wlog;

    gr_row = grd_bm.row;

    do {
        if ((d = fix_ceil(tli->right.y) - fix_ceil(tli->left.y)) > 0) {

            d = fix_ceil(tli->left.y) - tli->left.y;

#if InvDiv
            inv_dy = fix_div(fix_make(1, 0), dy);
            du = fix_mul_asm_safe(du, inv_dy);
            dv = fix_mul_asm_safe(dv, inv_dy);
#else
            du = fix_div(du, dy);
            dv = fix_div(dv, dy);
#endif
            u += fix_mul(du, d);
            v += fix_mul(dv, d);

            p_dest = grd_bm.bits + (gr_row * fix_cint(tli->left.y)) + tli->x;
            y = fix_cint(tli->right.y) - fix_cint(tli->left.y);

            switch (tli->bm.hlog) {
            case GRL_OPAQUE:
                for (; y > 0; y--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    *p_dest = t_bits[k]; // gr_fill_upixel(t_bits[k],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS:
                for (; y > 0; y--) {
                    if (temp_pix = t_bits[t_vtab[fix_fint(v)] + fix_fint(u)])
                        *p_dest = temp_pix; // gr_fill_upixel(t_bits[k],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_OPAQUE | GRL_LOG2:
                for (; y > 0; y--) {
                    *p_dest =
                        t_bits[((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask]; // gr_fill_upixel(t_bits[k],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS | GRL_LOG2:
                for (; y > 0; y--) {
                    if (temp_pix = t_bits[((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask])
                        *p_dest = temp_pix; // gr_fill_upixel(t_bits[k],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_OPAQUE | GRL_CLUT:
                for (; y > 0; y--) {
                    *p_dest =
                        t_clut[t_bits[t_vtab[fix_fint(v)] + fix_fint(u)]]; // gr_fill_upixel(t_clut[t_bits[k]],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS | GRL_CLUT:
                for (; y > 0; y--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    if (k = t_bits[k])
                        *p_dest = t_clut[k]; // gr_fill_upixel(t_clut[k],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_OPAQUE | GRL_LOG2 | GRL_CLUT:
                for (; y > 0; y--) {
                    *p_dest = t_clut[t_bits[((fix_fint(v) << t_wlog) + fix_fint(u)) &
                                            t_mask]]; // gr_fill_upixel(t_clut[t_bits[k]],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS | GRL_LOG2 | GRL_CLUT:
                for (; y > 0; y--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    if (k = t_bits[k])
                        *p_dest = t_clut[k]; // gr_fill_upixel(t_clut[k],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                }
                break;
            }
        } else if (d < 0)
            return TRUE; /* punt this tmap */

        tli->w += tli->dw;

        // figure out new left u & v & i
        inv_dy = 0;
        k = tli->left.u + tli->left.du;
        y = tli->left.v + tli->left.dv;

#if InvDiv
        inv_dy = fix_div(fix_make(1, 0), tli->w);
        u = fix_mul_asm_safe(k, inv_dy);
        v = fix_mul_asm_safe(y, inv_dy);
#else
        u = fix_div(k, tli->w);
        v = fix_div(y, tli->w);
#endif

        tli->left.u = k;
        tli->left.v = y;

        // figure out new right u & v & i
        k = tli->right.u + tli->right.du;
        y = tli->right.v + tli->right.dv;

#if InvDiv
        du = fix_mul_asm_safe(k, inv_dy) - u;
        dv = fix_mul_asm_safe(y, inv_dy) - v;
#else
        du = fix_div(k, tli->w) - u;
        dv = fix_div(y, tli->w) - v;
#endif

        tli->right.u = k;
        tli->right.v = y;

        tli->left.y += tli->left.dy;
        tli->right.y += tli->right.dy;
        dy = tli->right.y - tli->left.y;
        tli->x++;

    } while (--(tli->n) > 0);

    return false;
}

void gri_trans_wall_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS;
    }
    tli->loop_func = (void (*)())gri_wall_umap_loop;
    tli->left_edge_func = (void (*)())gri_uvwy_edge;
    tli->right_edge_func = (void (*)())gri_uvwy_edge;
}

void gri_opaque_wall_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE;
    }
    tli->loop_func = (void (*)())gri_wall_umap_loop;
    tli->left_edge_func = (void (*)())gri_uvwy_edge;
    tli->right_edge_func = (void (*)())gri_uvwy_edge;
}

void gri_trans_clut_wall_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2 | GRL_CLUT;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS | GRL_CLUT;
    }
    tli->loop_func = (void (*)())gri_wall_umap_loop;
    tli->left_edge_func = (void (*)())gri_uvwy_edge;
    tli->right_edge_func = (void (*)())gri_uvwy_edge;
}

void gri_opaque_clut_wall_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2 | GRL_CLUT;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE | GRL_CLUT;
    }
    tli->loop_func = (void (*)())gri_wall_umap_loop;
    tli->left_edge_func = (void (*)())gri_uvwy_edge;
    tli->right_edge_func = (void (*)())gri_uvwy_edge;
}

/*extern "C"
{
extern int HandleWallLoop1D_PPC(grs_tmap_loop_info *tli,
                                                                                                                                fix u, fix v, fix dv, fix dy,
                                                                                                                                uchar *t_clut, long *t_vtab, uchar *o_bits,
                                                                                                                                long gr_row, ulong t_mask, ulong t_wlog);
}*/

int HandleWallLoop1D_C(grs_tmap_loop_info *tli, fix u, fix v, fix dv, fix dy, uchar *t_clut, long *t_vtab,
                       uchar *o_bits, long gr_row, ulong t_mask, ulong t_wlog) {
    register int k, y;
    register fix inv_dy;
    register uchar *grd_bits, *p_dest, *t_bits;
    register fix ry, ly;

    ry = tli->right.y;
    ly = tli->left.y;

    grd_bits = grd_bm.bits + tli->x;
    tli->x += tli->n;
    do {
        if ((k = fix_ceil(ry) - fix_ceil(ly)) > 0) {

            k = fix_ceil(ly) - ly;

            dv = fix_div(dv, dy);
            v += fix_mul(dv, k);

            p_dest = grd_bits + (gr_row * fix_cint(ly));
            y = fix_cint(ry) - fix_cint(ly);
            t_bits = o_bits + fix_fint(u);
            for (; y > 0; y--) {
                k = ((fix_fint(v) << t_wlog)) & t_mask;
                *p_dest = t_clut[t_bits[k]]; // gr_fill_upixel(t_clut[t_bits[k]],t_x,y);
                v += dv;
                p_dest += gr_row;
            }
        } else if (k < 0)
            return TRUE; // punt this tmap

        tli->w += tli->dw;

        // figure out new left u & v
        k = tli->left.u + tli->left.du;
        y = tli->left.v + tli->left.dv;

        inv_dy = fix_div(fix_make(1, 0), tli->w);
        u = fix_mul_asm_safe(k, inv_dy);
        v = fix_mul_asm_safe(y, inv_dy);

        tli->left.u = k;
        tli->left.v = y;

        // figure out new right u & v
        tli->right.u += tli->right.du;
        y = tli->right.v + tli->right.dv;
        dv = fix_mul_asm_safe(y, inv_dy) - v;
        tli->right.v = y;

        ly += tli->left.dy;
        ry += tli->right.dy;
        dy = ry - ly;
        grd_bits++;
    } while (--(tli->n) > 0);

    tli->right.y = ry;
    tli->left.y = ly;

    return false;
}

// ==================================================================
// 1D versions
int gri_wall_umap_loop_1D(grs_tmap_loop_info *tli) {
    fix u, v, dv, dy, d;

    // locals used to store copies of tli-> stuff, so its in registers on the PPC
    int k, y;
    uchar t_wlog;
    ulong t_mask;
    long *t_vtab;
    uchar *t_bits, *o_bits;
    uchar *p_dest;
    fix inv_dy;
    uchar temp_pix;
    uchar *t_clut;
    long gr_row;

#if InvDiv
    inv_dy = fix_div(fix_make(1, 0), tli->w);
    u = fix_mul_asm_safe(tli->left.u, inv_dy);
    v = fix_mul_asm_safe(tli->left.v, inv_dy);
    dv = fix_mul_asm_safe(tli->right.v, inv_dy) - v;
#else
    u = fix_div(tli->left.u, tli->w);
    v = fix_div(tli->left.v, tli->w);
    dv = fix_div(tli->right.v, tli->w) - v;
#endif

    dy = tli->right.y - tli->left.y;

    t_vtab = tli->vtab;
    o_bits = tli->bm.bits;

    t_clut = tli->clut;
    t_mask = tli->mask;
    t_wlog = tli->bm.wlog;

    gr_row = grd_bm.row;

    return HandleWallLoop1D_C(tli, u, v, dv, dy, t_clut, t_vtab, o_bits, gr_row, t_mask, t_wlog);
}

void gri_opaque_clut_wall1d_umap_init(grs_tmap_loop_info *tli) {
    // MLA - Wall1d is always log2
    /*   if ((tli->bm.row==(1<<tli->bm.wlog)) &&
                (tli->bm.h==(1<<tli->bm.hlog))) {*/
    tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
    tli->bm.hlog = GRL_OPAQUE | GRL_LOG2 | GRL_CLUT;
    /*   } else {
          tli->vtab=gr_make_vtab(&(tli->bm));
          tli->bm.hlog=GRL_OPAQUE|GRL_CLUT;
       }*/
    tli->loop_func = (void (*)())gri_wall_umap_loop_1D;
    tli->left_edge_func = (void (*)())gri_uvwy_edge;
    tli->right_edge_func = (void (*)())gri_uvwy_edge;
}
