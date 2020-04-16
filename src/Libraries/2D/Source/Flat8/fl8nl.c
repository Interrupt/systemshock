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
 * $Source: r:/prj/lib/src/2d/RCS/fl8nl.c $
 * $Revision: 1.3 $
 * $Author: kevin $
 * $Date: 1994/08/16 13:12:41 $
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
#include "plytyp.h"
#include "poly.h"
#include "tlucdat.h"
#include "tmapint.h"
#include "vtab.h"

// prototypes
int gri_tluc8_lin_umap_loop(grs_tmap_loop_info *tli);

int gri_tluc8_lin_umap_loop(grs_tmap_loop_info *tli)
{
  fix u = tli->left.u, du = tli->right.u - u;
  fix v = tli->left.v, dv = tli->right.v - v;
  int32_t *t_vtab = tli->vtab;
  uchar *t_bits = tli->bm.bits;
  uchar *t_clut = tli->clut;
  uint32_t t_mask = tli->mask;
  uchar t_wlog = tli->bm.wlog;
  uchar temp_pix;

  while (tli->n)
  {
    fix dx = tli->right.x - tli->left.x;
    if (dx <= 0) return TRUE; //might divide by zero below; punt this tmap

    uchar *p       = tli->d + fix_cint(tli->left.x);
    uchar *p_final = tli->d + fix_cint(tli->right.x);

    du = fix_div(du, dx);
    dv = fix_div(dv, dx);

    switch (tli->bm.hlog)
    {
      case GRL_OPAQUE:
        for (; p < p_final; p++)
        {
          int k = t_vtab[fix_fint(v)] + fix_fint(u);
          k = t_bits[k];
          if (tluc8tab[k] != NULL) *p = tluc8tab[k][*p]; else *p = k;
          u += du;
          v += dv;
        }
      break;

      case GRL_TRANS:
        for (; p < p_final; p++)
        {
          int k = t_vtab[fix_fint(v)] + fix_fint(u);
          k = t_bits[k];
          if (k != 0) {
            if (tluc8tab[k] != NULL) *p = tluc8tab[k][*p]; else *p = k;
          }
          u += du;
          v += dv;
        }
      break;

      case GRL_OPAQUE | GRL_LOG2:
        for (; p < p_final; p++)
        {
          int k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
          k = t_bits[k];
          if (tluc8tab[k] != NULL) *p = tluc8tab[k][*p]; else *p = k;
          u += du;
          v += dv;
        }
      break;

      case GRL_TRANS | GRL_LOG2:
        for (; p < p_final; p++)
        {
          int k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
          k = t_bits[k];
          if (k != 0) {
            if (tluc8tab[k] != NULL) *p = tluc8tab[k][*p]; else *p = k;
          }
          u += du;
          v += dv;
        }
      break;

      case GRL_OPAQUE | GRL_CLUT:
        for (; p < p_final; p++)
        {
          int k = t_vtab[fix_fint(v)] + fix_fint(u);
          k = t_bits[k];
          if (tluc8tab[k] != NULL) *p = t_clut[tluc8tab[k][*p]]; else *p = t_clut[k];
          u += du;
          v += dv;
        }
      break;

      case GRL_TRANS | GRL_CLUT:
        for (; p < p_final; p++)
        {
          int k = t_vtab[fix_fint(v)] + fix_fint(u);
          k = t_bits[k];
          if (k != 0) {
            if (tluc8tab[k] != NULL) *p = t_clut[tluc8tab[k][*p]]; else *p = t_clut[k];
          }
          u += du;
          v += dv;
        }
      break;

      case GRL_OPAQUE | GRL_LOG2 | GRL_CLUT:
        for (; p < p_final; p++)
        {
          int k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
          k = t_bits[k];
          if (tluc8tab[k] != NULL) *p = t_clut[tluc8tab[k][*p]]; else *p = t_clut[k];
          u += du;
          v += dv;
        }
      break;

      case GRL_TRANS | GRL_LOG2 | GRL_CLUT:
        for (; p < p_final; p++)
        {
          int k = ((fix_fint(v) << t_wlog) + fix_fint(u)) & t_mask;
          k = t_bits[k];
          if (k != 0) {
            if (tluc8tab[k] != NULL) *p = t_clut[tluc8tab[k][*p]]; else *p = t_clut[k];
          }
          u += du;
          v += dv;
        }
      break;
    }

    u = (tli->left.u += tli->left.du);
    tli->right.u += tli->right.du;
    du = tli->right.u - u;

    v = (tli->left.v += tli->left.dv);
    tli->right.v += tli->right.dv;
    dv = tli->right.v - v;

    tli->left.x += tli->left.dx;
    tli->right.x += tli->right.dx;

    tli->d += grd_bm.row;
    tli->n --;
  }

  return FALSE; //tmap OK
}

void gri_tluc8_trans_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS;
    }
    tli->d = grd_bm.bits + grd_bm.row * tli->y;
    tli->loop_func = (void (*)())gri_tluc8_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvx_edge;
    tli->left_edge_func = (void (*)())gri_uvx_edge;
}

void gri_tluc8_opaque_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE;
    }
    tli->d = grd_bm.bits + grd_bm.row * tli->y;
    tli->loop_func = (void (*)())gri_tluc8_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvx_edge;
    tli->left_edge_func = (void (*)())gri_uvx_edge;
}

void gri_tluc8_trans_clut_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_TRANS | GRL_LOG2 | GRL_CLUT;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_TRANS | GRL_CLUT;
    }
    tli->d = grd_bm.bits + grd_bm.row * tli->y;
    tli->loop_func = (void (*)())gri_tluc8_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvx_edge;
    tli->left_edge_func = (void (*)())gri_uvx_edge;
}

void gri_tluc8_opaque_clut_lin_umap_init(grs_tmap_loop_info *tli) {
    if ((tli->bm.row == (1 << tli->bm.wlog)) && (tli->bm.h == (1 << tli->bm.hlog))) {
        tli->mask = (1 << (tli->bm.hlog + tli->bm.wlog)) - 1;
        tli->bm.hlog = GRL_OPAQUE | GRL_LOG2 | GRL_CLUT;
    } else {
        tli->vtab = gr_make_vtab(&(tli->bm));
        tli->bm.hlog = GRL_OPAQUE | GRL_CLUT;
    }
    tli->d = grd_bm.bits + grd_bm.row * tli->y;
    tli->loop_func = (void (*)())gri_tluc8_lin_umap_loop;
    tli->right_edge_func = (void (*)())gri_uvx_edge;
    tli->left_edge_func = (void (*)())gri_uvx_edge;
}
