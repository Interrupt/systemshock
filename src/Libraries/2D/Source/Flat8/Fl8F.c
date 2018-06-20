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
 * $Source: r:/prj/lib/src/2d/RCS/FL8F.c $
 * $Revision: 1.4 $
 * $Author: kevin $
 * $Date: 1994/08/16 12:50:13 $
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
#include "tmapint.h"
#include "vtab.h"

int gri_floor_umap_loop(grs_tmap_loop_info *tli);

int gri_floor_umap_loop(grs_tmap_loop_info *tli) {
    fix u, v, du, dv, dx, d;

    // locals used to store copies of tli-> stuff, so its in registers on the PPC
    uchar t_wlog;
    ulong t_mask;
    int x, k;
    uchar *t_bits;
    uchar *p_dest;
    fix inv;
    uchar *t_clut;
    uchar temp_pix;
    long *t_vtab;

#if InvDiv
    inv = fix_div(fix_make(1, 0), tli->w);
    u = fix_mul_asm_safe(tli->left.u, inv);
    du = fix_mul_asm_safe(tli->right.u, inv) - u;
    v = fix_mul_asm_safe(tli->left.v, inv);
    dv = fix_mul_asm_safe(tli->right.v, inv) - v;
#else
    u = fix_div(tli->left.u, tli->w);
    du = fix_div(tli->right.u, tli->w) - u;
    v = fix_div(tli->left.v, tli->w);
    dv = fix_div(tli->right.v, tli->w) - v;
#endif

    dx = tli->right.x - tli->left.x;

    t_clut = tli->clut;
    t_mask = tli->mask;
    t_wlog = tli->bm.wlog;
    t_vtab = tli->vtab;
    t_bits = tli->bm.bits;

    // handle PowerPC loop
    do {
        if ((d = fix_ceil(tli->right.x) - fix_ceil(tli->left.x)) > 0) {
            d = fix_ceil(tli->left.x) - tli->left.x;

#if InvDiv
            inv = fix_div(fix_make(1, 0), dx);
            du = fix_mul_asm_safe(du, inv);
            dv = fix_mul_asm_safe(dv, inv);
#else
            du = fix_div(du, dx);
            dv = fix_div(dv, dx);
#endif

            u += fix_mul(du, d);
            v += fix_mul(dv, d);

            // copy out tli-> stuff into locals
            p_dest = grd_bm.bits + (grd_bm.row * tli->y) + fix_cint(tli->left.x);
            x = fix_cint(tli->right.x) - fix_cint(tli->left.x);

            switch (tli->bm.hlog) {
            case GRL_OPAQUE:
                for (; x > 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    *(p_dest++) = t_bits[k]; // gr_fill_upixel(t_bits[k],x,t_y);
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS:
                for (; x > 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    if (temp_pix = t_bits[k])
                        *p_dest = temp_pix;
                    // gr_fill_upixel(t_bits[k],x,t_y);
                    p_dest++;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_OPAQUE | GRL_LOG2:
                for (; x > 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    *(p_dest++) = t_bits[k];
                    // gr_fill_upixel(t_bits[k],x,t_y);
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS | GRL_LOG2:
                for (; x > 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    if (temp_pix = t_bits[k])
                        *p_dest = temp_pix;
                    // gr_fill_upixel(t_bits[k],x,t_y);
                    p_dest++;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_OPAQUE | GRL_CLUT:
                for (; x > 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    *(p_dest++) = t_clut[t_bits[k]];
                    // gr_fill_upixel(tli->clut[t_bits[k]],x,t_y);
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS | GRL_CLUT:
                for (; x > 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    if (k = t_bits[k])
                        *p_dest = t_clut[k];
                    // gr_fill_upixel(tli->clut[k],x,t_y);
                    p_dest++;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_OPAQUE | GRL_LOG2 | GRL_CLUT:
                while ((long)p_dest & 3 != 0) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    *(p_dest++) = t_clut[t_bits[k]];
                    // gr_fill_upixel(tli->clut[t_bits[k]],x,t_y);
                    u += du;
                    v += dv;
                    x--;
                }

                while (x >= 0) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    inv = t_clut[t_bits[k]];
                    // gr_fill_upixel(tli->clut[t_bits[k]],x,t_y);
                    u += du;
                    v += dv;
                    *p_dest = inv;
                    p_dest++;
                    x--;
                }

                for (; x > 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    *(p_dest++) = t_clut[t_bits[k]];
                    // gr_fill_upixel(tli->clut[t_bits[k]],x,t_y);
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS | GRL_LOG2 | GRL_CLUT:
                for (; x > 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    if (k = t_bits[k])
                        *p_dest = t_clut[k]; // gr_fill_upixel(tli->clut[k],x,t_y);
                    p_dest++;
                    u += du;
                    v += dv;
                }
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
#else
        u = fix_div((tli->left.u += tli->left.du), tli->w);
        tli->right.u += tli->right.du;
        du = fix_div(tli->right.u, tli->w) - u;
        v = fix_div((tli->left.v += tli->left.dv), tli->w);
        tli->right.v += tli->right.dv;
        dv = fix_div(tli->right.v, tli->w) - v;
#endif

        tli->left.x += tli->left.dx;
        tli->right.x += tli->right.dx;
        dx = tli->right.x - tli->left.x;
        tli->y++;
    } while (--(tli->n) > 0);
    return FALSE; /* tmap OK */
}

void gri_trans_floor_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS;
    }
    tli->loop_func = (void (*)())gri_floor_umap_loop;
    tli->left_edge_func = (void (*)())gri_uvwx_edge;
    tli->right_edge_func = (void (*)())gri_uvwx_edge;
}

void gri_opaque_floor_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE;
    }
    tli->loop_func = (void (*)())gri_floor_umap_loop;
    tli->left_edge_func = (void (*)())gri_uvwx_edge;
    tli->right_edge_func = (void (*)())gri_uvwx_edge;
}

void gri_trans_clut_floor_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2 | GRL_CLUT;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS | GRL_CLUT;
    }
    tli->loop_func = (void (*)())gri_floor_umap_loop;
    tli->left_edge_func = (void (*)())gri_uvwx_edge;
    tli->right_edge_func = (void (*)())gri_uvwx_edge;
}

void gri_opaque_clut_floor_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2 | GRL_CLUT;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE | GRL_CLUT;
    }
    tli->loop_func = (void (*)())gri_floor_umap_loop;
    tli->left_edge_func = (void (*)())gri_uvwx_edge;
    tli->right_edge_func = (void (*)())gri_uvwx_edge;
}
