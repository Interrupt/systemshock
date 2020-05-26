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
#include "poly.h"
#include "scrmac.h"
#include "tmapint.h"
#include "vtab.h"

int gri_lit_floor_umap_loop(grs_tmap_loop_info *tli);

int gri_lit_floor_umap_loop(grs_tmap_loop_info *tli) {

#if InvDiv
    fix inv = fix_div(fix_make(1, 0), tli->w);
    fix u = fix_mul_asm_safe(tli->left.u, inv);
    fix du = fix_mul_asm_safe(tli->right.u, inv) - u;
    fix v = fix_mul_asm_safe(tli->left.v, inv);
    fix dv = fix_mul_asm_safe(tli->right.v, inv) - v;
    fix i = fix_mul_asm_safe(tli->left.i, inv);
    fix di = fix_mul_asm_safe(tli->right.i, inv) - i;
#else
    fix u = fix_div(tli->left.u, tli->w);
    fix du = fix_div(tli->right.u, tli->w) - u;
    fix v = fix_div(tli->left.v, tli->w);
    fix dv = fix_div(tli->right.v, tli->w) - v;
    fix i = fix_div(tli->left.i, tli->w);
    fix di = fix_div(tli->right.i, tli->w) - i;
#endif

    ulong t_mask = tli->mask;
    uchar t_wlog = tli->bm.wlog;
    uchar *g_ltab = grd_screen->ltab;
    int32_t *t_vtab = tli->vtab;
    uchar *t_bits = tli->bm.bits;

    do {
        fix dx = tli->right.x - tli->left.x;
        if (dx > 0)
        {

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

            fix d = fix_ceil(tli->left.x) - tli->left.x;
            u += fix_mul(du, d);
            v += fix_mul(dv, d);
            i += fix_mul(di, d);

            uchar *p_dest = grd_bm.bits + (grd_bm.row * tli->y) + fix_cint(tli->left.x);

            int x = fix_cint(tli->right.x) - fix_cint(tli->left.x);

            switch (tli->bm.hlog) {
                case GRL_OPAQUE:
                    for (; x > 0; x--) {
                        int k = t_vtab[fix_fint(v)] + fix_fint(u);
                        *(p_dest++) = g_ltab[t_bits[k] + fix_light(i)];
                        // gr_fill_upixel(g_ltab[t_bits[k]+fix_light(i)],x,t_y);
                    }
                break;

                case GRL_TRANS:
                    for (; x > 0; x--) {
                        int k = t_vtab[fix_fint(v)] + fix_fint(u);
                        k = t_bits[k];
                        if (k != 0) *p_dest = g_ltab[k + fix_light(i)];
                        // gr_fill_upixel(g_ltab[k+fix_light(i)],x,t_y);
                        p_dest++;
                        u += du;
                        v += dv;
                        i += di;
                    }
                break;

                case GRL_OPAQUE|GRL_LOG2:
                    for (; x > 0; x--) {
                        int k = ((fix_fint(v)<<t_wlog)+fix_fint(u))&t_mask;
                        *(p_dest++) = g_ltab[t_bits[k]+fix_light(i)];
                        // gr_fill_upixel(g_ltab[t_bits[k]+fix_light(i)],x,t_y);
                        u += du;
                        v += dv;
                        i += di;
                    }
                break;

                case GRL_TRANS | GRL_LOG2:
                    for (; x > 0; x--) {
                        int k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                        k = t_bits[k];
                        if (k != 0) *p_dest = g_ltab[k + fix_light(i)];
                        // gr_fill_upixel(g_ltab[k+fix_light(i)],x,t_y);
                        p_dest++;
                        u += du;
                        v += dv;
                        i += di;
                    }
                break;
            }
        }

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

        tli->y++;

    } while (--(tli->n) > 0);

    return FALSE; // tmap OK
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
