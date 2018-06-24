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
 * $Source: r:/prj/lib/src/2d/RCS/genlf.c $
 * $Revision: 1.3 $
 * $Author: kevin $
 * $Date: 1994/08/16 12:50:14 $
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
#include "grpix.h"
#include "poly.h"
#include "scrmac.h"
#include "tmapint.h"
#include "vtab.h"

int gri_lit_floor_umap_loop(grs_tmap_loop_info *tli);

/*extern "C"
{
extern int HandleFloorLoop_PPC(grs_tmap_loop_info *tli,
                                                                                                                         fix u, fix v, fix du, fix dv, fix dx, fix i, fix di,
                                                                                                                         uchar t_wlog, ulong	t_mask, uchar *t_bits, uchar *g_ltab);
}*/

int HandleFloorLoop_C(grs_tmap_loop_info *tli, fix u, fix v, fix du, fix dv, fix dx, fix i, fix di, uchar t_wlog,
                      ulong t_mask, uchar *t_bits, uchar *g_ltab) {
    uchar *p_dest;
    int x;
    fix inv;

    do {
        if ((x = fix_ceil(tli->right.x) - fix_ceil(tli->left.x)) > 0) {
            x = fix_ceil(tli->left.x) - tli->left.x;

            inv = fix_div(fix_make(1, 0) << 8, dx);
            di = fix_mul_asm_safe_light(di, inv);
            inv >>= 8;
            du = fix_mul_asm_safe(du, inv);
            dv = fix_mul_asm_safe(dv, inv);

            u += fix_mul(du, x);
            v += fix_mul(dv, x);
            i += fix_mul(di, x);

            // copy out tli-> stuff into locals
            p_dest = grd_bm.bits + (grd_bm.row * tli->y) + fix_cint(tli->left.x);
            x = fix_cint(tli->right.x) - fix_cint(tli->left.x);

            while ((long)p_dest & 3 != 0) {
                *(p_dest++) = g_ltab[t_bits[((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask] + fix_light(i)];
                u += du;
                v += dv;
                i += di;
                x--;
            }

            while (x >= 0) {
                inv = g_ltab[t_bits[((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask] + fix_light(i)];
                u += du;
                v += dv;
                i += di;
                *p_dest = inv;
                p_dest++;
                x--;
            }

            for (; x > 0; x--) {
                *(p_dest++) = g_ltab[t_bits[((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask] + fix_light(i)];
                u += du;
                v += dv;
                i += di;
            }
        } else if (x < 0)
            return TRUE; // punt this tmap

        tli->w += tli->dw;

        inv = fix_div(fix_make(1, 0), tli->w);
        u = fix_mul_asm_safe((tli->left.u += tli->left.du), inv);
        tli->right.u += tli->right.du;
        du = fix_mul_asm_safe(tli->right.u, inv) - u;
        v = fix_mul_asm_safe((tli->left.v += tli->left.dv), inv);
        tli->right.v += tli->right.dv;
        dv = fix_mul_asm_safe(tli->right.v, inv) - v;
        i = fix_mul_asm_safe((tli->left.i += tli->left.di), inv);
        tli->right.i += tli->right.di;
        di = fix_mul_asm_safe(tli->right.i, inv) - i;

        tli->left.x += tli->left.dx;
        tli->right.x += tli->right.dx;
        dx = tli->right.x - tli->left.x;
        tli->y++;
    } while (--(tli->n) > 0);

    return FALSE; // tmap OK
}

int gri_lit_floor_umap_loop(grs_tmap_loop_info *tli) {
    fix u, v, i, du, dv, di, dx, d;

    // locals used to store copies of tli-> stuff, so its in registers on the PPC
    int x, k;
    uchar t_wlog;
    ulong t_mask;
    uchar *t_bits;
    uchar *p_dest;
    uchar *g_ltab;
    fix inv;
    long *t_vtab;

#if InvDiv
    inv = fix_div(fix_make(1, 0), tli->w);
    u = fix_mul_asm_safe(tli->left.u, inv);
    du = fix_mul_asm_safe(tli->right.u, inv) - u;
    v = fix_mul_asm_safe(tli->left.v, inv);
    dv = fix_mul_asm_safe(tli->right.v, inv) - v;
    i = fix_mul_asm_safe(tli->left.i, inv);
    di = fix_mul_asm_safe(tli->right.i, inv) - i;
#else
    u = fix_div(tli->left.u, tli->w);
    du = fix_div(tli->right.u, tli->w) - u;
    v = fix_div(tli->left.v, tli->w);
    dv = fix_div(tli->right.v, tli->w) - v;
    i = fix_div(tli->left.i, tli->w);
    di = fix_div(tli->right.i, tli->w) - i;
#endif

    dx = tli->right.x - tli->left.x;

    t_mask = tli->mask;
    t_wlog = tli->bm.wlog;
    g_ltab = grd_screen->ltab;

    t_vtab = tli->vtab;
    t_bits = tli->bm.bits;

    if (tli->bm.hlog == (GRL_OPAQUE | GRL_LOG2))
        return HandleFloorLoop_C(tli, u, v, du, dv, dx, i, di, t_wlog, t_mask, t_bits, g_ltab);

    do {
        if ((d = fix_ceil(tli->right.x) - fix_ceil(tli->left.x)) > 0) {
            d = fix_ceil(tli->left.x) - tli->left.x;

#if InvDiv
            inv = fix_div(fix_make(1, 0) << 8, dx);
            di = fix_mul_asm_safe_light(di, inv);
            inv >>= 8;
            du = fix_mul_asm_safe(du, inv);
            dv = fix_mul_asm_safe(dv, inv);
#else
            du = fix_div(du, dx);
            dv = fix_div(dv, dx);
            di = fix_div(di, dx);
#endif
            u += fix_mul(du, d);
            v += fix_mul(dv, d);
            i += fix_mul(di, d);

            // copy out tli-> stuff into locals
            p_dest = grd_bm.bits + (grd_bm.row * tli->y) + fix_cint(tli->left.x);
            x = fix_cint(tli->right.x) - fix_cint(tli->left.x);

            switch (tli->bm.hlog) {
            case GRL_OPAQUE:
                for (; x > 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    *(p_dest++) =
                        g_ltab[t_bits[k] + fix_light(i)]; // gr_fill_upixel(g_ltab[t_bits[k]+fix_light(i)],x,t_y);
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
                // special case handles this
                /*      case GRL_OPAQUE|GRL_LOG2:
                          for (; x>0; x--) {
                            k=((fix_fint(v)<<t_wlog)+fix_fint(u))&t_mask;
                              *(p_dest++) = g_ltab[t_bits[k]+fix_light(i)];		//
                   gr_fill_upixel(g_ltab[t_bits[k]+fix_light(i)],x,t_y); u+=du; v+=dv;
                   i+=di;
                         }
                         break;*/
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

        tli->w += tli->dw;

#if InvDiv
        inv = fix_div(fix_make(1, 0), tli->w);
        u = fix_mul_asm_safe((tli->left.u += tli->left.du), inv);
        tli->right.u += tli->right.du;
        du = fix_mul_asm_safe(tli->right.u, inv) - u;
        v = fix_mul_asm_safe((tli->left.v += tli->left.dv), inv);
        tli->right.v += tli->right.dv;
        dv = fix_mul_asm_safe(tli->right.v, inv) - v;
        i = fix_mul_asm_safe((tli->left.i += tli->left.di), inv);
        tli->right.i += tli->right.di;
        di = fix_mul_asm_safe(tli->right.i, inv) - i;
#else
        u = fix_div((tli->left.u += tli->left.du), tli->w);
        tli->right.u += tli->right.du;
        du = fix_div(tli->right.u, tli->w) - u;
        v = fix_div((tli->left.v += tli->left.dv), tli->w);
        tli->right.v += tli->right.dv;
        dv = fix_div(tli->right.v, tli->w) - v;
        i = fix_div((tli->left.i += tli->left.di), tli->w);
        tli->right.i += tli->right.di;
        di = fix_div(tli->right.i, tli->w) - i;
#endif

        tli->left.x += tli->left.dx;
        tli->right.x += tli->right.dx;
        dx = tli->right.x - tli->left.x;
        tli->y++;
    } while (--(tli->n) > 0);
    return FALSE; /* tmap OK */
}

void gri_trans_lit_floor_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS;
    }
    tli->loop_func = (void (*)())gri_lit_floor_umap_loop;
    tli->left_edge_func = (void (*)())gri_uviwx_edge;
    tli->right_edge_func = (void (*)())gri_uviwx_edge;
}

void gri_opaque_lit_floor_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE;
    }
    tli->loop_func = (void (*)())gri_lit_floor_umap_loop;
    tli->left_edge_func = (void (*)())gri_uviwx_edge;
    tli->right_edge_func = (void (*)())gri_uviwx_edge;
}
