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
 * $Source: r:/prj/lib/src/2d/RCS/fl8lw.c $
 * $Revision: 1.4 $
 * $Author: kevin $
 * $Date: 1994/08/16 12:50:16 $
 *
 * Routines to wall floor map a flat8 bitmap to a generic canvas.
 *
 * This file is part of the 2d library.
 *
 */

#include "cnvdat.h"
#include "fl8tf.h"
#include "fl8tmapdv.h"
#include "gente.h"
#include "grpix.h"
#include "poly.h"
#include "scrmac.h"
#include "tmapint.h"
#include "vtab.h"

int gri_lit_wall_umap_loop(grs_tmap_loop_info *tli);
int gri_lit_wall_umap_loop_1D(grs_tmap_loop_info *tli);

int gri_lit_wall_umap_loop(grs_tmap_loop_info *tli) {
    fix u, v, i, du, dv, di, dy, d;

    // locals used to store copies of tli-> stuff, so its in registers on the PPC
    int k, y;
    ulong t_mask;
    ulong t_wlog;
    uchar *t_bits;
    uchar *p_dest;
    long gr_row;
    uchar *g_ltab;
    fix inv_dy;
    long *t_vtab;

#if InvDiv
    inv_dy = fix_div(fix_make(1, 0), tli->w);
    u = fix_mul_asm_safe(tli->left.u, inv_dy);
    du = fix_mul_asm_safe(tli->right.u, inv_dy) - u;
    v = fix_mul_asm_safe(tli->left.v, inv_dy);
    dv = fix_mul_asm_safe(tli->right.v, inv_dy) - v;
    i = fix_mul_asm_safe(tli->left.i, inv_dy);
    di = fix_mul_asm_safe(tli->right.i, inv_dy) - i;
    if (di >= -256 && di <= 256)
        i += 1024;
#else
    u = fix_div(tli->left.u, tli->w);
    du = fix_div(tli->right.u, tli->w) - u;
    v = fix_div(tli->left.v, tli->w);
    dv = fix_div(tli->right.v, tli->w) - v;
    i = fix_div(tli->left.i, tli->w);
    di = fix_div(tli->right.i, tli->w) - i;
    if (di >= -256 && di <= 256)
        i += 1024;
#endif

    dy = tli->right.y - tli->left.y;

    t_mask = tli->mask;
    t_wlog = tli->bm.wlog;
    g_ltab = grd_screen->ltab;
    t_vtab = tli->vtab;
    t_bits = tli->bm.bits;
    gr_row = grd_bm.row;

    do {
        if ((d = fix_ceil(tli->right.y) - fix_ceil(tli->left.y)) > 0) {
            d = fix_ceil(tli->left.y) - tli->left.y;

#if InvDiv
            inv_dy = fix_div(fix_make(1, 0) << 8, dy);
            di = fix_mul_asm_safe_light(di, inv_dy);
            inv_dy >>= 8;
            du = fix_mul_asm_safe(du, inv_dy);
            dv = fix_mul_asm_safe(dv, inv_dy);
#else
            du = fix_div(du, dy);
            dv = fix_div(dv, dy);
            di = fix_div(di, dy);
#endif
            u += fix_mul(du, d);
            v += fix_mul(dv, d);
            i += fix_mul(di, d);

            y = fix_cint(tli->right.y) - fix_cint(tli->left.y);
            p_dest = grd_bm.bits + (gr_row * fix_cint(tli->left.y)) + tli->x;

            switch (tli->bm.hlog) {
            case GRL_OPAQUE:
                for (; y > 0; y--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    *p_dest = g_ltab[t_bits[k] + fix_light(i)]; // gr_fill_upixel(g_ltab[t_bits[k]+fix_light(i)],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                    i += di;
                }
                break;
            case GRL_TRANS:
                for (; y > 0; y--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    if (k = t_bits[k])
                        *p_dest = g_ltab[k + fix_light(i)]; // gr_fill_upixel(g_ltab[k+fix_light(i)],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                    i += di;
                }
                break;
            case GRL_OPAQUE | GRL_LOG2:
                for (; y > 0; y--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    *p_dest = g_ltab[t_bits[k] + fix_light(i)];
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                    i += di;
                }
                break;
            case GRL_TRANS | GRL_LOG2:
                for (; y > 0; y--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    if (k = t_bits[k])
                        *p_dest = g_ltab[k + fix_light(i)]; // gr_fill_upixel(g_ltab[k+fix_light(i)],t_x,y);
                    p_dest += gr_row;
                    u += du;
                    v += dv;
                    i += di;
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
        tli->left.i += tli->left.di;

#if InvDiv
        inv_dy = fix_div(fix_make(1, 0), tli->w);
        u = fix_mul_asm_safe(k, inv_dy);
        v = fix_mul_asm_safe(y, inv_dy);
        i = fix_mul_asm_safe(tli->left.i, inv_dy);
        if (di >= -256 && di <= 256)
            i += 1024;
#else
        u = fix_div(k, tli->w);
        v = fix_div(y, tli->w);
        i = fix_div(tli->left.i, tli->w);
        if (di >= -256 && di <= 256)
            i += 1024;
#endif

        tli->left.u = k;
        tli->left.v = y;

        // figure out new right u & v & i
        k = tli->right.u + tli->right.du;
        y = tli->right.v + tli->right.dv;
        tli->right.i += tli->right.di;

#if InvDiv
        du = fix_mul_asm_safe(k, inv_dy) - u;
        dv = fix_mul_asm_safe(y, inv_dy) - v;
        di = fix_mul_asm_safe(tli->right.i, inv_dy) - i;
#else
        du = fix_div(k, tli->w) - u;
        dv = fix_div(y, tli->w) - v;
        di = fix_div(tli->right.i, tli->w) - i;
#endif
        tli->right.u = k;
        tli->right.v = y;

        tli->left.y += tli->left.dy;
        tli->right.y += tli->right.dy;
        dy = tli->right.y - tli->left.y;
        tli->x++;
    } while (--(tli->n) > 0);

    return FALSE; /* tmap OK */
}

void gri_trans_lit_wall_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS;
    }
    tli->loop_func = (void (*)())gri_lit_wall_umap_loop;
    tli->left_edge_func = (void (*)())gri_uviwy_edge;
    tli->right_edge_func = (void (*)())gri_uviwy_edge;
}

void gri_opaque_lit_wall_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE;
    }
    tli->loop_func = (void (*)())gri_lit_wall_umap_loop;
    tli->left_edge_func = (void (*)())gri_uviwy_edge;
    tli->right_edge_func = (void (*)())gri_uviwy_edge;
}

/*extern "C"
{
extern int HandleWallLitLoop1D_PPC(grs_tmap_loop_info *tli,
                                                                                                                                                fix u, fix v, fix i, fix dv, fix di, fix dy,
                                                                                                                                                uchar *g_ltab, long *t_vtab, uchar *o_bits,
                                                                                                                                                long gr_row, ulong t_mask, ulong t_wlog);
}*/

int HandleWallLitLoop1D_C(grs_tmap_loop_info *tli, fix u, fix v, fix i, fix dv, fix di, fix dy, uchar *g_ltab,
                          uchar *o_bits, long gr_row, ulong t_mask, ulong t_wlog) {
    fix d, inv_dy;
    register fix lefty, righty;
    long k, y;
    uchar *t_bits;
    uchar *p_dest;

    lefty = tli->left.y;
    righty = tli->right.y;
    do {
        if ((d = fix_ceil(righty) - fix_ceil(lefty)) > 0) {
            d = fix_ceil(lefty) - lefty;

            inv_dy = fix_div(fix_make(1, 0) << 8, dy);
            dv = fix_mul_asm_safe(dv, inv_dy >> 8);
            di = fix_mul_asm_safe_light(di, inv_dy);

            v += fix_mul(dv, d);
            i += fix_mul(di, d);

            if (di >= -256 && di <= 256)
                i += 256;

            y = fix_cint(righty) - fix_cint(lefty);
            p_dest = grd_bm.bits + (gr_row * fix_cint(lefty)) + tli->x;
            t_bits = o_bits + fix_fint(u);

            // inner loop
            for (; y > 0; y--) {
                k = (fix_fint(v) << t_wlog) & t_mask;
                *p_dest = g_ltab[t_bits[k] + fix_light(i)];
                p_dest += gr_row;
                v += dv;
                i += di;
            }

        } else if (d < 0)
            return TRUE; // punt this tmap

        tli->w += tli->dw;

        // figure out new left u & v & i
        k = tli->left.u + tli->left.du;
        y = tli->left.v + tli->left.dv;
        tli->left.i += tli->left.di;

        inv_dy = fix_div(fix_make(1, 0), tli->w);
        u = fix_mul_asm_safe(k, inv_dy);
        v = fix_mul_asm_safe(y, inv_dy);
        i = fix_mul_asm_safe(tli->left.i, inv_dy);

        tli->left.u = k;
        tli->left.v = y;

        // figure out new right u & v & i
        k = tli->right.u + tli->right.du;
        y = tli->right.v + tli->right.dv;
        tli->right.i += tli->right.di;

        dv = fix_mul_asm_safe(y, inv_dy) - v;
        di = fix_mul_asm_safe(tli->right.i, inv_dy) - i;
        if (di >= -256 && di <= 256)
            i += 1024;

        tli->right.u = k;
        tli->right.v = y;

        lefty += tli->left.dy;
        righty += tli->right.dy;
        dy = righty - lefty;
        tli->x++;
    } while (--(tli->n) > 0);

    tli->left.y = lefty;
    tli->right.y = righty;

    return FALSE; // tmap OK
}

// ==================================================================================
// Wall_1D versions of routines
int gri_lit_wall_umap_loop_1D(grs_tmap_loop_info *tli) {
    fix u, v, i, dv, di, dy;

    // locals used to store copies of tli-> stuff, so its in registers on the PPC
    int k, y;
    ulong t_mask;
    ulong t_wlog;
    long gr_row;
    uchar *g_ltab;
    uchar *o_bits;
    fix inv_dy;

#if InvDiv
    inv_dy = fix_div(fix_make(1, 0), tli->w);
    u = fix_mul_asm_safe(tli->left.u, inv_dy);
    v = fix_mul_asm_safe(tli->left.v, inv_dy);
    dv = fix_mul_asm_safe(tli->right.v, inv_dy) - v;
    i = fix_mul_asm_safe(tli->left.i, inv_dy);
    di = fix_mul_asm_safe(tli->right.i, inv_dy) - i;
    if (di >= -256 && di <= 256)
        i += 512;
#else
    u = fix_div(tli->left.u, tli->w);
    v = fix_div(tli->left.v, tli->w);
    dv = fix_div(tli->right.v, tli->w) - v;
    i = fix_div(tli->left.i, tli->w);
    di = fix_div(tli->right.i, tli->w) - i;
    if (di >= -256 && di <= 256)
        i += 512;
#endif

    dy = tli->right.y - tli->left.y;

    t_mask = tli->mask;
    t_wlog = tli->bm.wlog;
    g_ltab = grd_screen->ltab;
    o_bits = tli->bm.bits;
    gr_row = grd_bm.row;

    return HandleWallLitLoop1D_C(tli, u, v, i, dv, di, dy, g_ltab, o_bits, gr_row, t_mask, t_wlog);
}

void gri_opaque_lit_wall1d_umap_init(grs_tmap_loop_info *tli) {
    // Wall1D is always log2
    /*   if ((tli->bm.row==(1<<tli->bm.wlog)) &&
                (tli->bm.h==(1<<tli->bm.hlog))) {*/
    tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
    tli->bm.hlog = GRL_OPAQUE | GRL_LOG2;
    /*   } else {
          tli->vtab=gr_make_vtab(&(tli->bm));
          tli->bm.hlog=GRL_OPAQUE;
       }*/
    tli->loop_func = (void (*)())gri_lit_wall_umap_loop_1D;
    tli->left_edge_func = (void (*)())gri_uviwy_edge;
    tli->right_edge_func = (void (*)())gri_uviwy_edge;
}
