/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.
Copyright (C) 2018-2020 Shockolate Project

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
 *  $Source: r:/prj/lib/src/2d/RCS/lintab.c $
 *  $Revision: 1.5 $
 *  $Author: kevin $
 *  $Date: 1994/10/04 18:46:52 $
 */

#include "grnull.h"
#include "lintyp.h"
#include "linfcn.h"

/* in addition to the tables that are in the list, there are device specific tables */

grt_uline_fill *grd_uline_fill_vector;
grt_uline_fill_table *grd_uline_fill_table;

grt_uline_fill flat8_uline_fill_table[GRD_FILL_TYPES][GRD_LINE_TYPES] = {
    // FILL_NORM
    {
        gri_flat8_uline_ns,                 // GR_LINE
        gri_all_uiline_fill,                // GR_ILINE
        gri_flat8_uhline_ns,                // GR_HLINE
        gri_flat8_uvline_ns,                // GR_VLINE
        gri_flat8_usline_norm,              // GR_SLINE
        gri_flat8_ucline_norm,              // GR_CLINE
        gri_flat8_wire_poly_uline,          // GR_WIRE_POLY_LINE
        gr_null,                            // GR_WIRE_POLY_SLINE
        gri_flat8_wire_poly_ucline_norm     // GR_WIRE_POLY_CLINE
    },
    // FILL_CLUT
    {
        gri_flat8_uline_clut,
        gri_all_uiline_fill,
        gri_flat8_uhline_clut,
        gri_flat8_uvline_clut,
        gri_flat8_usline_clut,
        gri_flat8_ucline_norm,
        gri_flat8_wire_poly_uline,
        gr_null,
        gri_flat8_wire_poly_ucline_clut
    },
    // FILL_XOR
    {
        gri_flat8_uline_xor,
        gri_all_uiline_fill,
        gri_flat8_uhline_xor,
        gri_flat8_uvline_xor,
        gri_flat8_usline_xor,
        gri_flat8_ucline_norm,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    // FILL_BLEND
    {
        gri_flat8_uline_blend,
        gri_all_uiline_fill,
        gri_flat8_uhline_blend,
        gri_flat8_uvline_blend,
        gri_flat8_usline_blend,
        gri_flat8_ucline_norm,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    // FILL_SOLID
    {
        gri_flat8_uline_ns,
        gri_all_uiline_fill,
        gri_flat8_uhline_ns,
        gri_flat8_uvline_ns,
        gri_flat8_uline_ns,
        gri_flat8_uline_ns,
        gri_flat8_wire_poly_uline,
        gr_null,
        gri_flat8_wire_poly_uline
    },
};

/* gen have their fill types info buries in grd_fill_pixel */
grt_uline_fill gen_uline_fill_table[GRD_FILL_TYPES][GRD_LINE_TYPES] = {
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_uline_fill,
        gri_gen_uline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
};

/* WH - these tables not used by game
// temporarily modex, bank8 and bank24 are all gen's
grt_uline_fill modex_uline_fill_table[GRD_FILL_TYPES][GRD_LINE_TYPES] = {
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        (void *)gr_null, // MLA gri_modex_uhline_ns,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        (void *)gr_null, // MLA		gri_modex_uhline_ns,
        gri_gen_uvline_fill,
        gri_gen_uline_fill,
        gri_gen_uline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
};

// WH - this too
grt_uline_fill bank8_uline_fill_table[GRD_FILL_TYPES][GRD_LINE_TYPES] = {
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        (void *)gr_null, // MLA		gri_bank8_uhline_ns,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        (void *)gr_null, // MLA		gri_bank8_uhline_ns,
        gri_gen_uvline_fill,
        gri_gen_uline_fill,
        gri_gen_uline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
};

// WH - and this
grt_uline_fill bank24_uline_fill_table[GRD_FILL_TYPES][GRD_LINE_TYPES] = {
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_usline_fill,
        gri_gen_ucline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
    {
        gri_gen_uline_fill,
        gri_all_uiline_fill,
        gri_gen_uhline_fill,
        gri_gen_uvline_fill,
        gri_gen_uline_fill,
        gri_gen_uline_fill,
        gri_gen_wire_poly_uline,
        gr_null,
        gri_gen_wire_poly_ucline
    },
};
*/

grt_uline_fill_table *grd_uline_fill_table_list[] = {
    NULL,
    NULL,
    (grt_uline_fill_table *)flat8_uline_fill_table,
    NULL,
#ifdef GR_DOUBLE_CANVAS
    (grt_uline_fill_table *)flat8_uline_fill_table,
#else
    NULL,
#endif
    NULL,
    NULL,
    (grt_uline_fill_table *)gen_uline_fill_table
};
