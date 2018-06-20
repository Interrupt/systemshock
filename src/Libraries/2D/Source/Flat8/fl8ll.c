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
 * $Source: r:/prj/lib/src/2d/RCS/genll.c $
 * $Revision: 1.3 $
 * $Author: kevin $
 * $Date: 1994/08/16 12:50:11 $
 *
 * Routines to linearly texture map a flat8 bitmap to a generic canvas.
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
#include "scrdat.h"
#include "tmapint.h"
#include "vtab.h"

int gri_lit_lin_umap_loop(grs_tmap_loop_info *tli);

// PPC specific optimized routines
/*extern "C"
{
int Handle_Lit_Lin_Loop_PPC(fix u, fix v, fix du, fix dv, fix dx,
                                                                                                                grs_tmap_loop_info
*tli, uchar *start_pdest, uchar *t_bits, long gr_row, fix i, fix di, uchar *g_ltab, uchar	t_wlog, ulong	t_mask);

int Handle_TLit_Lin_Loop2_PPC(fix u, fix v, fix du, fix dv, fix dx,
                                                                                                                        grs_tmap_loop_info *tli, uchar *start_pdest, uchar *t_bits, long gr_row,
                                                                                                                        fix i, fix di, uchar *g_ltab, uchar	t_wlog, ulong	t_mask);
}*/

int Handle_Lit_Lin_Loop_C(fix u, fix v, fix du, fix dv, fix dx, grs_tmap_loop_info *tli, uchar *start_pdest,
                          uchar *t_bits, long gr_row, fix i, fix di, uchar *g_ltab, uchar t_wlog, ulong t_mask) {
    int x, t_xl, t_xr, inv;
    uchar *p_dest;

    tli->y += tli->n;

    do {
        if ((x = fix_ceil(tli->right.x) - fix_ceil(tli->left.x)) > 0) {
            x = fix_div(fix_make(1, 0) << 8, dx);
            di = fix_mul_asm_safe_light(di, x);
            x >>= 8;
            du = fix_mul_asm_safe(du, x);
            dv = fix_mul_asm_safe(dv, x);

            x = fix_ceil(tli->left.x) - tli->left.x;
            u += fix_mul(du, x);
            v += fix_mul(dv, x);
            i += fix_mul(di, x);

            // copy out tli-> stuff into locals
            t_xl = fix_cint(tli->left.x);
            t_xr = fix_cint(tli->right.x);
            p_dest = start_pdest + t_xl;
            x = t_xr - t_xl;

            for (; x > 0; x--) {
                *(p_dest++) = g_ltab[t_bits[((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask] + fix_light(i)];
                u += du;
                v += dv;
                i += di;
            }
        } else if (x < 0)
            return TRUE; // punt this tmap

        u = (tli->left.u += tli->left.du);
        tli->right.u += tli->right.du;
        du = tli->right.u - u;
        v = (tli->left.v += tli->left.dv);
        tli->right.v += tli->right.dv;
        dv = tli->right.v - v;
        i = (tli->left.i += tli->left.di);
        tli->right.i += tli->right.di;
        di = tli->right.i - i;
        tli->left.x += tli->left.dx;
        tli->right.x += tli->right.dx;
        dx = tli->right.x - tli->left.x;
        start_pdest += gr_row;
    } while (--(tli->n) > 0);
    return FALSE; // tmap OK
}

int Handle_TLit_Lin_Loop2_C(fix u, fix v, fix du, fix dv, fix dx, grs_tmap_loop_info *tli, uchar *start_pdest,
                            uchar *t_bits, long gr_row, fix i, fix di, uchar *g_ltab, uchar t_wlog, ulong t_mask) {
    int x, k;
    uchar *p_dest;
    int t_xl, t_xr;
    int lx, rx;

    lx = tli->left.x;
    rx = tli->right.x;

    tli->y += tli->n;
    do {
        if ((x = fix_ceil(rx) - fix_ceil(lx)) > 0) {
            x = fix_ceil(lx) - lx;

            k = fix_div(fix_make(1, 0) << 8, dx);
            di = fix_mul_asm_safe_light(di, k);
            k >>= 8;
            du = fix_mul_asm_safe(du, k);
            dv = fix_mul_asm_safe(dv, k);

            u += fix_mul(du, x);
            v += fix_mul(dv, x);
            i += fix_mul(di, x);

            // copy out tli-> stuff into locals
            t_xl = fix_cint(lx);
            t_xr = fix_cint(rx);
            p_dest = start_pdest + t_xl;
            x = t_xr - t_xl;

            for (; x > 0; x--) {
                // assume pixel in transparent first
                k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                k = t_bits[k];
                if (k) // not transparent, move to assuming opaque
                    *p_dest = g_ltab[k + fix_light(i)];

                p_dest++;
                u += du;
                v += dv;
                i += di;
            }
        } else if (x < 0)
            return TRUE; // punt this tmap

        u = (tli->left.u += tli->left.du);
        tli->right.u += tli->right.du;
        du = tli->right.u - u;
        v = (tli->left.v += tli->left.dv);
        tli->right.v += tli->right.dv;
        dv = tli->right.v - v;
        i = (tli->left.i += tli->left.di);
        tli->right.i += tli->right.di;
        di = tli->right.i - i;
        lx += tli->left.dx;
        rx += tli->right.dx;
        dx = rx - lx;
        start_pdest += gr_row;
    } while (--(tli->n) > 0);

    tli->left.x = lx;
    tli->right.x = rx;

    return FALSE; // tmap OK
}

int gri_lit_lin_umap_loop(grs_tmap_loop_info *tli) {
    fix u, v, i, du, dv, di, dx, d;

    // locals used to store copies of tli-> stuff, so its in registers on the PPC
    register int x, k;
    int t_xl, t_xr, inv;
    long *t_vtab;
    uchar *t_bits;
    uchar *p_dest;
    uchar temp_pix;
    uchar t_wlog;
    ulong t_mask;
    uchar *g_ltab;
    long gr_row;
    uchar *start_pdest;

    u = tli->left.u;
    du = tli->right.u - u;
    v = tli->left.v;
    dv = tli->right.v - v;
    i = tli->left.i;
    di = tli->right.i - i;
    dx = tli->right.x - tli->left.x;

    t_vtab = tli->vtab;
    t_mask = tli->mask;
    t_wlog = tli->bm.wlog;
    g_ltab = grd_screen->ltab;

    t_bits = tli->bm.bits;
    gr_row = grd_bm.row;
    start_pdest = grd_bm.bits + (gr_row * (tli->y));

    // handle optimized cases first
    if (tli->bm.hlog == (GRL_OPAQUE | GRL_LOG2))
        return (
            Handle_Lit_Lin_Loop_C(u, v, du, dv, dx, tli, start_pdest, t_bits, gr_row, i, di, g_ltab, t_wlog, t_mask));
    if (tli->bm.hlog == (GRL_TRANS | GRL_LOG2))
        return (
            Handle_TLit_Lin_Loop2_C(u, v, du, dv, dx, tli, start_pdest, t_bits, gr_row, i, di, g_ltab, t_wlog, t_mask));

    do {
        if ((d = fix_ceil(tli->right.x) - fix_ceil(tli->left.x)) > 0) {
            d = fix_ceil(tli->left.x) - tli->left.x;

#if InvDiv
            k = fix_div(fix_make(1, 0) << 8, dx);
            di = fix_mul_asm_safe_light(di, k);
            k >>= 8;
            du = fix_mul_asm_safe(du, k);
            dv = fix_mul_asm_safe(dv, k);
#else
            du = fix_div(du, dx);
            dv = fix_div(dv, dx);
            di = fix_div(di, dx);
#endif

            u += fix_mul(du, d);
            v += fix_mul(dv, d);
            i += fix_mul(di, d);

            // copy out tli-> stuff into locals
            t_xl = fix_cint(tli->left.x);
            t_xr = fix_cint(tli->right.x);
            p_dest = start_pdest + t_xl;
            x = t_xr - t_xl;

            switch (tli->bm.hlog) {
            case GRL_OPAQUE:
                for (; x > 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    *(p_dest++) =
                        g_ltab[t_bits[k] + fix_light(i)]; // gr_fill_upixel(g_ltab[t_bits[k]+fix_light(i)],x,t_y);
                    u += du;
                    v += dv;
                    i += di;
                }
                break;
            case GRL_TRANS:
                for (; x > 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    if (k = t_bits[k])
                        *p_dest = g_ltab[k + fix_light(i)]; // gr_fill_upixel(g_ltab[k+fix_light(i)],x,t_y);
                    p_dest++;
                    u += du;
                    v += dv;
                    i += di;
                }
                break;
            // handled in special case code
            case GRL_OPAQUE | GRL_LOG2:
                for (; x > 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    *(p_dest++) =
                        g_ltab[t_bits[k] + fix_light(i)]; // gr_fill_upixel(g_ltab[t_bits[k]+fix_light(i)],x,t_y);
                    u += du;
                    v += dv;
                    i += di;
                }
                break;
            case GRL_TRANS | GRL_LOG2:
                for (; x > 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    if (k = t_bits[k])
                        *p_dest = g_ltab[k + fix_light(i)]; // gr_fill_upixel(g_ltab[k+fix_light(i)],x,t_y);
                    p_dest++;
                    u += du;
                    v += dv;
                    i += di;
                }
                break;
            }
        } else if (d < 0)
            return TRUE; /* punt this tmap */

        u = (tli->left.u += tli->left.du);
        tli->right.u += tli->right.du;
        du = tli->right.u - u;
        v = (tli->left.v += tli->left.dv);
        tli->right.v += tli->right.dv;
        dv = tli->right.v - v;
        i = (tli->left.i += tli->left.di);
        tli->right.i += tli->right.di;
        di = tli->right.i - i;
        tli->left.x += tli->left.dx;
        tli->right.x += tli->right.dx;
        dx = tli->right.x - tli->left.x;
        tli->y++;
        start_pdest += gr_row;
    } while (--(tli->n) > 0);
    return FALSE; /* tmap OK */
}

void gri_trans_lit_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS;
    }
    tli->loop_func = (void (*)())gri_lit_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvix_edge;
    tli->left_edge_func = (void (*)())gri_uvix_edge;
}

void gri_opaque_lit_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE;
    }

    tli->loop_func = (void (*)())gri_lit_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvix_edge;
    tli->left_edge_func = (void (*)())gri_uvix_edge;
}
