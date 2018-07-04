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
//
// $Source: r:/prj/lib/src/2d/RCS/fl8s.asm $
// $Revision: 1.1 $
// $Author: kevin $
// $Date: 1994/08/16 12:34:32 $
//
// Inner loops of scaling and clut scaling primitives.
//
// This file is part of the 2d library.
//

#include "cnvdat.h"
#include "flat8.h"
#include "gente.h"
#include "grnull.h"
#include "grpix.h"
#include "poly.h"
#include "tmapint.h"

// globals
long ADD_DEST_OFF;
long ADD_DV_FRAC_OFF;
long AND_BM_ROW_OFF;
long ADD_SRC_OFF;
long SET_REPS_OFF;
long SET_OFFSET_OFF;
long JMP_LOOP_MIDDLE_OFF;

#define unroll_num 4
#define unroll_log 2

// externs
extern int gri_poly_loop(grs_tmap_loop_info *ti);

// internal prototypes
int gri_scale_umap_loop_PPC(grs_tmap_loop_info *tli);
int gri_scale_umap_loop_68K(grs_tmap_loop_info *tli);

// This file contains the scalers for both 68K and PowerPC
// First the routines that are generic to both, then the PowerPC routines, then
// 68K

// ------------------------------------------------------------------------
// Generic (68K & PowerPC) routines
// ------------------------------------------------------------------------

// ========================================================================
// opaque solid polygon scaler
int gri_opaque_solid_scale_umap_init(grs_tmap_loop_info *info, grs_vertex **vert) {
    info->left_edge_func = (void (*)())gri_scale_edge;
    info->right_edge_func = (void (*)())gr_null;
    info->bm.hlog = 0;
    info->bm.bits = info->clut;
    info->loop_func = (void (*)())gri_poly_loop;
    info->d = ((uchar *)((long)grd_canvas->bm.row * (long)info->y));
    info->d += (long)grd_canvas->bm.bits;
    return (0);
}

// ------------------------------------------------------------------------
// PowerPC routines
// ------------------------------------------------------------------------
// ========================================================================
// transparent solid polygon scaler
int gri_trans_solid_scale_umap_init(grs_tmap_loop_info *tli, grs_vertex **vert) {
    tli->bm.hlog = GRL_TRANS | GRL_SOLID;
    tli->loop_func = (void (*)())gri_scale_umap_loop_PPC;
    tli->right_edge_func = gr_null;
    tli->left_edge_func = (void (*)())gri_scale_edge;
    return (0);
}

// ========================================================================
// transparent bitmap scaler
int gri_trans_scale_umap_init(grs_tmap_loop_info *tli, grs_vertex **vert) {
    tli->bm.hlog = GRL_TRANS;
    tli->loop_func = (void (*)())gri_scale_umap_loop_PPC;
    tli->right_edge_func = gr_null;
    tli->left_edge_func = (void (*)())gri_scale_edge;
    return (0);
}

// ========================================================================
// opaque bitmap scaler
int gri_opaque_scale_umap_init(grs_tmap_loop_info *tli) {
    tli->bm.hlog = GRL_OPAQUE;
    tli->loop_func = (void (*)())gri_scale_umap_loop_PPC;
    tli->right_edge_func = gr_null;
    tli->left_edge_func = (void (*)())gri_scale_edge;
    return (0);
}

// ========================================================================
// transparent clut bitmap scaler
int gri_trans_clut_scale_umap_init(grs_tmap_loop_info *tli) {
    tli->bm.hlog = GRL_TRANS | GRL_CLUT;
    tli->loop_func = (void (*)())gri_scale_umap_loop_PPC;
    tli->right_edge_func = gr_null;
    tli->left_edge_func = (void (*)())gri_scale_edge;
    return (0);
}

// ========================================================================
// opaque clut bitmap scaler
int gri_opaque_clut_scale_umap_init(grs_tmap_loop_info *tli) {
    tli->bm.hlog = GRL_OPAQUE | GRL_CLUT;
    tli->loop_func = (void (*)())gri_scale_umap_loop_PPC;
    tli->right_edge_func = gr_null;
    tli->left_edge_func = (void (*)())gri_scale_edge;
    return (0);
}

// ========================================================================
// main inside loop for PPC scalers
int gri_scale_umap_loop_PPC(grs_tmap_loop_info *tli) {
    fix u, ul, du;
    int x;
    uchar k;
    fix xl, xr, dx, d;
    uchar *p_src, *p_dest;

    xl = fix_cint(tli->left.x);
    xr = fix_cint(tli->right.x);
    if (xr <= xl)
        return TRUE;
    ul = tli->left.u;
    dx = tli->right.x - tli->left.x;
    du = fix_div(tli->right.u - ul, dx);
    d = fix_ceil(tli->left.x) - tli->left.x;
    ul += fix_mul(du, d);

    do {
        p_src = tli->bm.bits + tli->bm.row * fix_int(tli->left.v);
        p_dest = grd_bm.bits + (grd_bm.row * tli->y) + xl;
        switch (tli->bm.hlog) {
        case GRL_OPAQUE:
            for (x = xl, u = ul; x < xr; x++) {
                *(p_dest++) = p_src[fix_fint(u)]; // gr_fill_upixel(k,x,tli->y);
                u += du;
            }
            break;
        case GRL_TRANS:
            for (x = xl, u = ul; x < xr; x++) {
                if (k = p_src[fix_fint(u)])
                    *p_dest = k; // gr_fill_upixel(k,x,tli->y);
                u += du;
                p_dest++;
            }
            break;
        case GRL_OPAQUE | GRL_CLUT:
            for (x = xl, u = ul; x < xr; x++) {
                *(p_dest++) = tli->clut[p_src[fix_fint(u)]]; // gr_fill_upixel(tli->clut[k],x,tli->y);
                u += du;
            }
            break;
        case GRL_TRANS | GRL_CLUT:
            for (x = xl, u = ul; x < xr; x++) {
                if (k = p_src[fix_fint(u)])
                    *p_dest = tli->clut[k]; // gr_fill_upixel(tli->clut[k],x,tli->y);
                u += du;
                p_dest++;
            }
            break;
        case GRL_TRANS | GRL_SOLID:
            for (x = xl, u = ul; x < xr; x++) {
                if (k = p_src[fix_fint(u)])
                    *p_dest = (uchar)(tli->clut); // gr_fill_upixel((uchar )(tli->clut),x,tli->y);
                u += du;
                p_dest++;
            }
            break;
        }
        tli->left.v += tli->left.dv;
        tli->y++;
    } while (--(tli->n) > 0);

    return FALSE; /* tmap OK */
}
