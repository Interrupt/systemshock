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
// $Source: r:/prj/lib/src/3d/RCS/interp.asm $
// $Revision: 1.19 $
// $Author: jaemz $
// $Date: 1994/10/13 20:51:43 $
//
// 3d object interpreter
//
// $Log: interp.asm $
// Revision 1.19  1994/10/13  20:51:43  jaemz
// Fixed lighting bug
//
// Revision 1.18  1994/09/20  13:34:36  jaemz
// Lighting
//
// Revision 1.14  1994/08/18  03:45:30  jaemz
// Added stereo to objects for real, reevals bsp tree
//
// Revision 1.13  1994/07/15  14:13:34  jaemz
// Added _view_position with an underscore to make it c readable
//
// Revision 1.12  1994/05/19  09:46:39  kevin
// g3_draw_tmap now uses watcom register parameter passing conventions.
//
// Revision 1.11  1994/02/08  20:46:17  kaboom
// Updated usage of gour_flag.
//
// Revision 1.10  1993/11/18  10:08:11  dc
// first set of debug setup for the interpreter
//
// Revision 1.9  1993/10/25  16:24:46  kaboom
// Changed call to polygon routine to use new calling convention.
//
// Revision 1.8  1993/10/02  09:27:49  kaboom
// Added vtext_tab.  Also updated texture map opcode to call the uv perspective
// mapper.
//
// Revision 1.7  1993/09/15  04:01:23  dc
// tmap interface, well, except there isnt a tmapper
//
// Revision 1.6  1993/08/10  22:54:13  dc
// add _3d.inc to includes
//
// Revision 1.5  1993/08/04  00:47:10  dc
// support for new interpreter opcodes
//
// Revision 1.4  1993/06/03  14:34:00  matt
// Removed int -> sfix conversion in defres_i & setshade
//
// Revision 1.3  1993/06/02  16:57:01  matt
// Gouraud polys handled differently: gouraud base now added at poly draw
// time, not point definition time.
//
// Revision 1.2  1993/05/27  18:11:46  matt
// Added getparms opcodes, and changed parameter passing scheme.
//
// Revision 1.1  1993/05/04  17:39:50  matt
// Initial revision
//
//

#include "3d.h"
#include "GlobalV.h"
#include "lg.h"
//#include <String.h>
//#include <_stdarg.h>
//#include <stdarg.h>

// prototypes;
uchar *do_eof(uchar *);
uchar *do_jnorm(uchar *);
uchar *do_ldjnorm(uchar *);
uchar *do_ljnorm(uchar *);
uchar *do_lnres(uchar *);
uchar *do_multires(uchar *);
uchar *do_polyres(uchar *);
uchar *do_setcolor(uchar *);
uchar *do_sortnorm(uchar *);
uchar *do_debug(uchar *);
uchar *do_setshade(uchar *);
uchar *do_goursurf(uchar *);
uchar *do_x_rel(uchar *);
uchar *do_y_rel(uchar *);
uchar *do_z_rel(uchar *);
uchar *do_xy_rel(uchar *);
uchar *do_xz_rel(uchar *);
uchar *do_yz_rel(uchar *);
uchar *do_icall_p(uchar *);
uchar *do_icall_b(uchar *);
uchar *do_icall_h(uchar *);
uchar *do_sfcal(uchar *);
uchar *do_defres(uchar *);
uchar *do_defres_i(uchar *);
uchar *do_getparms(uchar *);
uchar *do_getparms_i(uchar *);
uchar *do_gour_p(uchar *);
uchar *do_gour_vc(uchar *);
uchar *do_getvcolor(uchar *);
uchar *do_getvscolor(uchar *);
uchar *do_rgbshades(uchar *);
uchar *do_draw_mode(uchar *);
uchar *do_getpcolor(uchar *);
uchar *do_getpscolor(uchar *);
uchar *do_scaleres(uchar *);
uchar *do_vpnt_p(uchar *);
uchar *do_vpnt_v(uchar *);
uchar *do_setuv(uchar *);
uchar *do_uvlist(uchar *);
uchar *do_tmap_op(uchar *);
uchar *do_dbg(uchar *);

extern int check_and_draw_common(long c, int n_verts, g3s_phandle *p);
extern int draw_poly_common(long c, int n_verts, g3s_phandle *p);
extern void g3_light_obj(g3s_phandle norm, g3s_phandle pos);

void interpreter_loop(uchar *object);

// globals
extern char gour_flag; // gour flag for actual polygon drawer

#define OP_EOF 0
#define OP_JNORM 1

#define n_ops 40
    void *opcode_table[n_ops] = {
        do_eof,        do_jnorm,     do_lnres,     do_multires,   do_polyres,    do_setcolor, do_sortnorm,
        do_debug,      do_setshade,  do_goursurf,  do_x_rel,      do_y_rel,      do_z_rel,    do_xy_rel,
        do_xz_rel,     do_yz_rel,    do_icall_p,   do_icall_b,    do_icall_h,    0,           do_sfcal,
        do_defres,     do_defres_i,  do_getparms,  do_getparms_i, do_gour_p,     do_gour_vc,  do_getvcolor,
        do_getvscolor, do_rgbshades, do_draw_mode, do_getpcolor,  do_getpscolor, do_scaleres, do_vpnt_p,
        do_vpnt_v,     do_setuv,     do_uvlist,    do_tmap_op,    do_dbg};

#define N_RES_POINTS 1000
#define PARM_DATA_SIZE 4 * 100

#define N_VCOLOR_ENTRIES 32
#define N_VPOINT_ENTRIES 32
#define N_VTEXT_ENTRIES 64

// This determines when we no longer reevaluate
// the bsp tree.  It corresponds to the tan of
// 7.12 degrees, which empirically seems fine
// we might have to set it lower some day if
// you see polygons drop out in stereo
#define STEREO_DIST_LIM = 0x2000

g3s_point *resbuf[N_RES_POINTS];
g3s_point *poly_buf[100];

// clang-format off
uchar _vcolor_tab[N_VCOLOR_ENTRIES] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
g3s_point *_vpoint_tab[N_VPOINT_ENTRIES] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
grs_bitmap *_vtext_tab[N_VTEXT_ENTRIES] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
// clang-format on

// ptr to stack parms
ubyte *parm_ptr; //va_list parm_ptr;

// space for parms to objects
char parm_data[PARM_DATA_SIZE];

char _itrp_gour_flg = 0;
char _itrp_wire_flg = 0;
char _itrp_check_flg = 0;

// MLA not used, - uchar 	*struct_ptr;

// c callable context setting routines

// takes ptr to object
// this is bullshit, man, takes ptr to object on the freakin' stack!
void g3_interpret_object(ubyte *object_ptr, ...) {
    int i, scale;

    // lighting stuff, params are on the stack
    // so don't sweat it
    // set fill type so 2d can light the thang
    if ((_g3d_light_type & (LT_SPEC | LT_DIFF)) != 0) {
        gr_set_fill_type(FILL_CLUT);
        if (_g3d_light_type == LT_DIFF)
            opcode_table[OP_JNORM] = &do_ldjnorm;
        else
            opcode_table[OP_JNORM] = &do_ljnorm;
    }

    // get addr of stack parms
    parm_ptr = (&object_ptr) + sizeof(object_ptr); //va_start(parm_ptr, object_ptr);

    // mark res points as free
    LG_memset(resbuf, 0, N_RES_POINTS * 4);

    // scale view vector for scale
    scale = *(short *)(object_ptr - 2);
    if (scale) {
        if (scale > 0) {
            _view_position.gX >>= scale;
            _view_position.gY >>= scale;
            _view_position.gZ >>= scale;
        } else {
            int temp;

            scale = -scale;

            temp = (((ulong)_view_position.gX) >> 16); // get high 16 bits
            // FIXME: DG: I guess they meant &, not &&
            // shamaz: Fixed that
            if (((temp << scale) & 0xffff0000) != 0)
                return;                             // overflow
            temp = (((ulong)_view_position.gY) >> 16); // get high 16 bits
            if (((temp << scale) & 0xffff0000) != 0)
                return;                             // overflow
            temp = (((ulong)_view_position.gZ) >> 16); // get high 16 bits
            if (((temp << scale) & 0xffff0000) != 0)
                return; // overflow

            _view_position.gX <<= scale;
            _view_position.gY <<= scale;
            _view_position.gZ <<= scale;
        }
    }

    interpreter_loop(object_ptr);

    // free res points
    for (i = N_RES_POINTS - 1; i >= 0; i--)
        if (resbuf[i])
            freepnt(resbuf[i]);

    // set lighting back to how it was
    if ((_g3d_light_type & (LT_SPEC | LT_DIFF)) != 0) {
        gr_set_fill_type(FILL_NORM);
        opcode_table[OP_JNORM] = &do_jnorm;
    }

}

// interpret the object
void interpreter_loop(uchar *object) {
    do {
        object = ((uchar * (*)(uchar *)) opcode_table[*(short *)object])(object);
    } while (object);
}

// opcodes.  [ebp] points at op on entry
uchar *do_debug(uchar *opcode) { return 0; }

uchar *do_eof(uchar *opcode) // and return extra level
{
    return 0;
}

// jnorm lbl,px,py,pz,nx,ny,nz
// v=viewer coords-p
// if (n*v)<0 then branch to lbl
uchar *do_jnorm(uchar *opcode) {
    if (g3_check_normal_facing((g3s_vector *)(opcode + 16), (g3s_vector *)(opcode + 4)))
        return opcode + 28; // surface is visible. continue
    else
        return opcode + (*(short *)(opcode + 2)); // surface not visible
}

// lnres pnt0,pnt1
uchar *do_lnres(uchar *opcode) {
    g3_draw_line(resbuf[*(unsigned short *)(opcode + 2)], resbuf[*(unsigned short *)(opcode + 4)]);
    return opcode + 6;
}

uchar *do_multires(uchar *opcode) {
    short count;

    count = *(short *)(opcode + 2);

    g3_transform_list(count, (g3s_phandle *)(resbuf + (*(short *)(opcode + 4))), (g3s_vector *)(opcode + 6));
    return opcode + 6 + (count * 12);
}

// this should do some cute matrix transform trick, not this ugly hack

// that kid from the wrong side came over my house again, decapitated all my
// dolls
//  and if you bore me, you lose your soul to me       - "Gepetto", Belly,
//  _Star_
uchar *do_scaleres(uchar *opcode) {
    // MLA - this routine appears to be buggy and can't possibly work, so I'm not
    // doing it yet.
    DEBUG("%s Call Mark!", __FUNCTION__);

    /* 	int					count,scale;
            long				temp_pnt[3];
            g3s_phandle	temp_hand;

            count = * (unsigned short *) (opcode+2);
            scale = * (unsigned short *) (opcode+4);
            temp_hand = (g3s_phandle) (parm_data+(* (unsigned short *)
       (opcode+6)));

            opcode += 8;
            do
             {
             }
            while (--count>0);

            return opcode;
             */
    return 0;
}

// these put the address of an old point in the interpreter respnt array
// note they will get freed when the interpreter punts
uchar *do_vpnt_p(uchar *opcode) {
    resbuf[*(short *)(opcode + 4)] = (g3s_point *)(*(long *)(parm_data + (*(unsigned short *)(opcode + 2))));
    return opcode + 6;
}

uchar *do_vpnt_v(uchar *opcode) {
    resbuf[*(short *)(opcode + 4)] = _vpoint_tab[(*(unsigned short *)(opcode + 2)) >> 2];
    return opcode + 6;
}

uchar *do_defres(uchar *opcode) {
    resbuf[*(unsigned short *)(opcode + 2)] = g3_transform_point((g3s_vector *)(opcode + 4));
    return opcode + 16;
}

uchar *do_defres_i(uchar *opcode) {
    g3s_phandle temphand;

    temphand = g3_transform_point((g3s_vector *)(opcode + 4));
    resbuf[*(unsigned short *)(opcode + 2)] = temphand;

    temphand->i = *(short *)(opcode + 16);
    temphand->p3_flags |= PF_I;

    return opcode + 18;
}

// polyres cnt,pnt0,pnt1,...
uchar *do_polyres(uchar *opcode) {
    int count, count2;

    count2 = count = *(unsigned short *)(opcode + 2);
    opcode += 4;
    while (--count >= 0) {
        poly_buf[count] = resbuf[*(unsigned short *)(opcode + (count << 1))];
    }

    opcode += count2 << 1;

    gour_flag = _itrp_gour_flg;
    if ((_itrp_check_flg & 1) == 0)
        draw_poly_common(gr_get_fcolor(), count2, poly_buf);
    else
        check_and_draw_common(gr_get_fcolor(), count2, poly_buf);

    return opcode;
}

uchar *do_sortnorm(uchar *opcode) {
    if (g3_check_normal_facing((g3s_vector *)(opcode + 14), (g3s_vector *)(opcode + 2))) {
        interpreter_loop(opcode + (*(short *)(opcode + 26)));
        interpreter_loop(opcode + (*(short *)(opcode + 28)));
    } else {
        interpreter_loop(opcode + (*(short *)(opcode + 28)));
        interpreter_loop(opcode + (*(short *)(opcode + 26)));
    }

    return opcode + 30;
}

uchar *do_goursurf(uchar *opcode) {
    gouraud_base = (*(short *)(opcode + 2)) << 8;
    _itrp_gour_flg = 2;
    return opcode + 4;
}

uchar *do_gour_p(uchar *opcode) {
    gouraud_base = parm_data[(*(short *)(opcode + 2))] << 8;
    _itrp_gour_flg = 2;
    return opcode + 4;
}

uchar *do_gour_vc(uchar *opcode) {
    gouraud_base = ((long)_vcolor_tab[*(unsigned short *)(opcode + 2)]) << 8;
    _itrp_gour_flg = 2;
    return opcode + 4;
}

uchar *do_draw_mode(uchar *opcode) {
    short flags;

    flags = *(short *)(opcode + 2);
    _itrp_wire_flg = flags >> 8;
    flags &= 0x00ff;
    flags <<= 1;
    _itrp_check_flg = flags >> 8;
    flags &= 0x00ff;
    flags <<= 2;
    _itrp_gour_flg = flags - 1;
    return opcode + 4;
}

uchar *do_setshade(uchar *opcode) {
    int i;
    uchar *new_opcode;
    g3s_phandle temphand;

    i = *(unsigned short *)(opcode + 2); // get number of shades
    new_opcode = opcode + 4 + (i << 2);

    while (--i >= 0) {
        temphand = resbuf[*(unsigned short *)(opcode + 4 + (i << 2))]; // get point handle
        temphand->i = *(short *)(opcode + 6 + (i << 2));
        temphand->p3_flags |= PF_I;
    }

    return new_opcode;
}

uchar *do_rgbshades(uchar *opcode) {
    uchar *new_opcode;
    int i;
    g3s_phandle temphand;

    i = *(unsigned short *)(opcode + 2); // get number of shades
    new_opcode = opcode + 4;
    while (--i >= 0) {
        temphand = resbuf[*(unsigned short *)new_opcode]; // get point handle
        temphand->rgb = *(long *)(new_opcode + 2);
        temphand->p3_flags |= PF_RGB;
        new_opcode += 10;
    }
    return new_opcode;
}

uchar *do_setuv(uchar *opcode) {
    g3s_phandle temphand;

    temphand = resbuf[*(unsigned short *)(opcode + 2)]; // get point handle
    temphand->uv.u = (*(unsigned long *)(opcode + 4)) >> 8;
    temphand->uv.v = (*(unsigned long *)(opcode + 8)) >> 8;
    temphand->p3_flags |= PF_U | PF_V;

    return opcode + 12;
}

uchar *do_uvlist(uchar *opcode) {
    int i;
    g3s_phandle temphand;

    i = *(unsigned short *)(opcode + 2); // get number of shades
    opcode += 4;
    while (--i >= 0) {
        temphand = resbuf[*(unsigned short *)opcode]; // get point handle
        temphand->uv.u = (*(unsigned long *)(opcode + 2)) >> 8;
        temphand->uv.v = (*(unsigned long *)(opcode + 6)) >> 8;
        temphand->p3_flags |= PF_U | PF_V;
        opcode += 10;
    }

    return opcode;
}

// should we be hacking _itrp_gour_flg?
uchar *do_setcolor(uchar *opcode) {
    gr_set_fcolor(*(unsigned short *)(opcode + 2));
    _itrp_gour_flg = 0;
    return opcode + 4;
}

uchar *do_getvcolor(uchar *opcode) {
    gr_set_fcolor(_vcolor_tab[*(unsigned short *)(opcode + 2)]);
    _itrp_gour_flg = 0;
    return opcode + 4;
}

uchar *do_getpcolor(uchar *opcode) {
    gr_set_fcolor(*(unsigned short *)(parm_data + (*(unsigned short *)(opcode + 2))));
    _itrp_gour_flg = 0;
    return opcode + 4;
}

uchar *do_getvscolor(uchar *opcode) {
    short temp;

    temp = (byte)_vcolor_tab[*(unsigned short *)(opcode + 2)];
    temp |= (*(short *)(opcode + 4)) << 8;
    gr_set_fcolor(gr_get_light_tab()[temp]);
    return opcode + 6;
}

uchar *do_getpscolor(uchar *opcode) {
    short temp;

    temp = (unsigned short)parm_data[*(unsigned short *)(opcode + 2)];
    temp &= 0x00ff;
    temp |= (*(short *)(opcode + 4)) << 8;
    gr_set_fcolor(gr_get_light_tab()[temp]);
    return opcode + 6;
}

uchar *do_x_rel(uchar *opcode) {
    resbuf[*(short *)(opcode + 2)] = g3_copy_add_delta_x(resbuf[*(short *)(opcode + 4)], *(fix *)(opcode + 6));
    return opcode + 10;
}

uchar *do_y_rel(uchar *opcode) {
    resbuf[*(short *)(opcode + 2)] = g3_copy_add_delta_y(resbuf[*(short *)(opcode + 4)], *(fix *)(opcode + 6));
    return opcode + 10;
}

uchar *do_z_rel(uchar *opcode) {
    resbuf[*(short *)(opcode + 2)] = g3_copy_add_delta_z(resbuf[*(short *)(opcode + 4)], *(fix *)(opcode + 6));
    return opcode + 10;
}

uchar *do_xy_rel(uchar *opcode) {
    resbuf[*(short *)(opcode + 2)] =
        g3_copy_add_delta_xy(resbuf[*(short *)(opcode + 4)], *(fix *)(opcode + 6), *(fix *)(opcode + 10));
    return opcode + 14;
}

uchar *do_xz_rel(uchar *opcode) {
    resbuf[*(short *)(opcode + 2)] =
        g3_copy_add_delta_xz(resbuf[*(short *)(opcode + 4)], *(fix *)(opcode + 6), *(fix *)(opcode + 10));
    return opcode + 14;
}

uchar *do_yz_rel(uchar *opcode) {
    resbuf[*(short *)(opcode + 2)] =
        g3_copy_add_delta_yz(resbuf[*(short *)(opcode + 4)], *(fix *)(opcode + 6), *(fix *)(opcode + 10));
    return opcode + 14;
}

uchar *do_icall_p(uchar *opcode) {
    g3_start_object_angles_x((g3s_vector *)(opcode + 6), *(fixang *)(parm_data + (*(unsigned short *)(opcode + 18))));
    interpreter_loop((uchar *)(*(long *)(opcode + 2)));
    g3_end_object();

    return opcode + 20;
}

uchar *do_icall_h(uchar *opcode) {
    g3_start_object_angles_y((g3s_vector *)(opcode + 6), *(fixang *)(parm_data + (*(unsigned short *)(opcode + 18))));
    interpreter_loop((uchar *)(*(long *)(opcode + 2)));
    g3_end_object();

    return opcode + 20;
}

uchar *do_icall_b(uchar *opcode) {
    g3_start_object_angles_z((g3s_vector *)(opcode + 6), *(fixang *)(parm_data + (*(unsigned short *)(opcode + 18))));
    interpreter_loop((uchar *)(*(long *)(opcode + 2)));
    g3_end_object();

    return opcode + 20;
}

uchar *do_sfcal(uchar *opcode) {
    interpreter_loop(opcode + (*(unsigned short *)(opcode + 2)));
    return opcode + 4;
}

// copy parms of stack. takes offset,count
uchar *do_getparms(uchar *opcode) {
    long *src, *dest;
    int count;

    dest = (long *)(parm_data + (*(unsigned short *)(opcode + 2)));
    src = (long *)(parm_ptr + (*(unsigned short *)(opcode + 4)));
    count = *(unsigned short *)(opcode + 6);
    while (count-- > 0)
        *(dest++) = *(src)++;

    return opcode + 8;
}

// copy parm block. ptr is on stack. takes dest_ofs,src_ptr_ofs,size
uchar *do_getparms_i(uchar *opcode) {
    long *src, *dest;
    int count;

    dest = *(long **)(parm_data + (*(unsigned short *)(opcode + 2)));
    src = (long *)(parm_ptr + (*(unsigned short *)(opcode + 4)));
    count = *(unsigned short *)(opcode + 6);
    while (count-- > 0)
        *(dest++) = *(src)++;

    return opcode + 8;
}

uchar *do_dbg(uchar *opcode) {
    return opcode + 8;
}

extern void (*g3_tmap_func)();
extern int temp_poly(long c, int n, grs_vertex **vpl);

uchar *do_tmap_op(uchar *opcode) {
    int count, count2;
    short temp;

    count2 = count = *(unsigned short *)(opcode + 4);
    count--;
    do {
        temp = *(short *)(opcode + 6 + (count << 1));

        poly_buf[count] = resbuf[temp];
    } while (--count >= 0);

    ((int (*)(int, g3s_phandle *, grs_bitmap *)) * g3_tmap_func)(count2, poly_buf,
                                                                 _vtext_tab[*(unsigned short *)(opcode + 2)]);

    return opcode + 6 + (count2 * 2);
}

// routines to shade objects
// mostly replacements for jnorm
// ljnorm lbl,px,py,pz,nx,ny,nz
// v=viewer coords-p
// if (n*v)<0 then branch to lbl
// does lit version of jnorm, for flat lighting
uchar *do_ljnorm(uchar *opcode) {
    if (g3_check_normal_facing((g3s_vector *)(opcode + 16), (g3s_vector *)(opcode + 4))) {
        g3_light_obj((g3s_phandle)(opcode + 4), (g3s_phandle)(opcode + 16));
        return opcode + 28;
    } else
        return opcode + (*(short *)(opcode + 2)); // surface not visible
}

// light diff not near norm
uchar *do_ldjnorm(uchar *opcode) {
    fix temp;

    if (g3_check_normal_facing((g3s_vector *)(opcode + 16), (g3s_vector *)(opcode + 4))) {
        temp = g3_vec_dotprod(&_g3d_light_vec, (g3s_vector *)(opcode + 4));
        temp <<= 1;
        if (temp < 0)
            temp = 0;
        temp += _g3d_amb_light;
        temp >>= 4;
        temp &= 0x0ffffff00;
        temp += _g3d_light_tab;
        gr_set_fill_parm(temp);

        return opcode + 28;
    } else
        return opcode + (*(short *)(opcode + 2)); // surface not visible
}

//external calls to these do-nothing functions can be safely removed
void FlipShort(short *sh) {}
void FlipLong(long *lng) {}
void FlipVector(short n, g3s_vector *vec) {}
