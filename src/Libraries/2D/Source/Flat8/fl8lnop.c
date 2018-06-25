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
 * $Source: r:/prj/lib/src/2d/RCS/genl.c $
 * $Revision: 1.4 $
 * $Author: kevin $
 * $Date: 1994/08/16 12:48:26 $
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
#include "tmapint.h"
#include "vtab.h"

// prototypes
int gri_lin_umap_loop(grs_tmap_loop_info *tli);

/*extern "C"
{
int Handle_LinClut_Loop_PPC(fix u, fix v, fix du, fix dv, fix dx,
                                                                                                                grs_tmap_loop_info
*tli, uchar *start_pdest, uchar *t_bits, long gr_row, uchar *t_clut, uchar	t_wlog, ulong t_mask);
}*/

int Handle_LinClut_Loop_C(fix u, fix v, fix du, fix dv, fix dx, grs_tmap_loop_info *tli, uchar *start_pdest,
                          uchar *t_bits, long gr_row, uchar *t_clut, uchar t_wlog, ulong t_mask) {
    register int x, k;
    uchar *p_dest;
    register fix rx, lx;

    rx = tli->right.x;
    lx = tli->left.x;
    tli->y += tli->n;

    do {
        if ((x = fix_ceil(rx) - fix_ceil(lx)) > 0) {
            x = fix_ceil(lx) - lx;

            k = fix_div(fix_make(1, 0), dx);
            du = fix_mul_asm_safe(du, k);
            dv = fix_mul_asm_safe(dv, k);

            u += fix_mul(du, x);
            v += fix_mul(dv, x);

            // copy out tli-> stuff into locals
            p_dest = start_pdest + fix_cint(lx);
            x = fix_cint(rx) - fix_cint(lx);

            for (; x > 0; x--) {
                *(p_dest++) = t_clut[t_bits[((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask]];
                u += du;
                v += dv;
            }
        } else if (x < 0)
            return TRUE; // punt this tmap

        u = (tli->left.u += tli->left.du);
        tli->right.u += tli->right.du;
        du = tli->right.u - u;
        v = (tli->left.v += tli->left.dv);
        tli->right.v += tli->right.dv;
        dv = tli->right.v - v;
        lx += tli->left.dx;
        rx += tli->right.dx;
        dx = rx - lx;
        start_pdest += gr_row;
    } while (--(tli->n) > 0);

    tli->right.x = rx;
    tli->left.x = lx;

    return FALSE; // tmap OK
}

int gri_lin_umap_loop(grs_tmap_loop_info *tli) {
    fix u, v, du, dv, dx, d;

    // locals used to store copies of tli-> stuff, so its in registers on the PPC
    register int x, k;
    uchar *p_dest;
    uchar temp_pix;
    long *t_vtab;
    uchar *t_bits;
    uchar *t_clut;
    uchar t_wlog;
    ulong t_mask;
    long gr_row;
    uchar *start_pdest;
    long inv;

    u = tli->left.u;
    du = tli->right.u - u;
    v = tli->left.v;
    dv = tli->right.v - v;
    dx = tli->right.x - tli->left.x;

    t_vtab = tli->vtab;
    t_clut = tli->clut;
    t_mask = tli->mask;
    t_wlog = tli->bm.wlog;

    t_bits = tli->bm.bits;
    gr_row = grd_bm.row;
    start_pdest = grd_bm.bits + (gr_row * (tli->y));

    // handle PowerPC loop
    if (tli->bm.hlog == (GRL_OPAQUE | GRL_LOG2 | GRL_CLUT))
        return (Handle_LinClut_Loop_C(u, v, du, dv, dx, tli, start_pdest, t_bits, gr_row, t_clut, t_wlog, t_mask));

    do {
        if ((d = fix_ceil(tli->right.x) - fix_ceil(tli->left.x)) > 0) {
            d = fix_ceil(tli->left.x) - tli->left.x;

#if InvDiv
            k = fix_div(fix_make(1, 0), dx);
            du = fix_mul_asm_safe(du, k);
            dv = fix_mul_asm_safe(dv, k);
#else
            du = fix_div(du, dx);
            dv = fix_div(dv, dx);
#endif
            u += fix_mul(du, d);
            v += fix_mul(dv, d);

            // copy out tli-> stuff into locals
            p_dest = start_pdest + fix_cint(tli->left.x);
            x = fix_cint(tli->right.x) - fix_cint(tli->left.x);

            switch (tli->bm.hlog) {
            case GRL_OPAQUE:
                for (; x > 0 && v >= 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    *(p_dest++) = t_bits[k]; // gr_fill_upixel(t_bits[k],x,y);
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS:
                for (; x > 0 && v >= 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    if (temp_pix = t_bits[k])
                        *p_dest = temp_pix; // gr_fill_upixel(t_bits[k],x,y);
                    p_dest++;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_OPAQUE | GRL_LOG2:
                for (; x > 0 && v >= 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    *(p_dest++) = t_bits[k]; // gr_fill_upixel(t_bits[k],x,y);
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS | GRL_LOG2:
                for (; x > 0 && v >= 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    if (temp_pix = t_bits[k])
                        *p_dest = temp_pix; // gr_fill_upixel(t_bits[k],x,y);
                    p_dest++;
                    u += du;
                    v += dv;
                }
                break;
            case GRL_OPAQUE | GRL_CLUT:
                for (; x > 0 && v >= 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    *(p_dest++) = t_clut[t_bits[k]]; // gr_fill_upixel(tli->clut[t_bits[k]],x,y);
                    u += du;
                    v += dv;
                }
                break;
            case GRL_TRANS | GRL_CLUT:
                for (; x > 0 && v >= 0; x--) {
                    k = t_vtab[fix_fint(v)] + fix_fint(u);
                    if (k = t_bits[k])
                        *p_dest = t_clut[k]; // gr_fill_upixel(tli->clut[k],x,y);
                    p_dest++;
                    u += du;
                    v += dv;
                }
                break;
                // handled in special case now
                /*     case GRL_OPAQUE|GRL_LOG2|GRL_CLUT:
                        for (; x>0; x--) {
                           k=((fix_fint(v)<<t_wlog)+fix_fint(u))&t_mask;
                           *(p_dest++) = t_clut[t_bits[k]];		//
                   gr_fill_upixel(tli->clut[t_bits[k]],x,y); u+=du; v+=dv;
                        }
                        break;*/
            case GRL_TRANS | GRL_LOG2 | GRL_CLUT:
                for (; x > 0 && v >= 0; x--) {
                    k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
                    if (k = t_bits[k])
                        *p_dest = t_clut[k]; // gr_fill_upixel(tli->clut[k],x,y);
                    p_dest++;
                    u += du;
                    v += dv;
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
        tli->left.x += tli->left.dx;
        tli->right.x += tli->right.dx;
        dx = tli->right.x - tli->left.x;
        tli->y++;
        start_pdest += gr_row;
    } while (--(tli->n) > 0);
    return FALSE; /* tmap OK */
}

void gri_trans_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS;
    }
    tli->loop_func = (void (*)())gri_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvx_edge;
    tli->left_edge_func = (void (*)())gri_uvx_edge;
}

void gri_opaque_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE;
    }
    tli->loop_func = (void (*)())gri_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvx_edge;
    tli->left_edge_func = (void (*)())gri_uvx_edge;
}

void gri_trans_clut_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2 | GRL_CLUT;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS | GRL_CLUT;
    }
    tli->loop_func = (void (*)())gri_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvx_edge;
    tli->left_edge_func = (void (*)())gri_uvx_edge;
}

void gri_opaque_clut_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2 | GRL_CLUT;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE | GRL_CLUT;
    }
    tli->loop_func = (void (*)())gri_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvx_edge;
    tli->left_edge_func = (void (*)())gri_uvx_edge;
}
