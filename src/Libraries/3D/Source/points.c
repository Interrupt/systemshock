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
// $Source: r:/prj/lib/src/3d/RCS/points.asm $
// $Revision: 1.17 $
// $Author: jaemz $
// $Date: 1994/09/28 19:00:52 $
//
// Point definition routines
//

//#include <FixMath.h>
#include "3d.h"
#include "GlobalV.h"
#include "lg.h"

// prototypes
void rotate_norm(g3s_vector *v, fix *x, fix *y, fix *z);
void do_norm_rotate(fix x, fix y, fix z, fix *rx, fix *ry, fix *rz);

void do_rotate(fix x, fix y, fix z, fix *rx, fix *ry, fix *rz);

// void xlate_rotate_point(g3s_vector *v, fix *x, fix *y, fix *z);
#define xlate_rotate_point(v, x, y, z) \
    do_rotate(v->gX - _view_position.gX, v->gY - _view_position.gY, v->gZ - _view_position.gZ, x, y, z)

extern int code_point(g3s_point *pt);
extern char SubLongWithOverflow(int32_t *result, int32_t src, int32_t dest);
extern bool AddLongWithOverflow(int32_t *result, int32_t src, int32_t dest);

// for temp use in rotate_list, etc.
g3s_codes g_codes;

// rotate a normal or gradient vector. esi=vector, returns edi=point, bl=codes.
// assumes perspective mapper scale factor will be set to _scrw.
g3s_phandle g3_rotate_norm(g3s_vector *v) {
    fix x, y, z;
    fix temp, temp2, temp3;
    g3s_point *point;

    rotate_norm(v, &x, &y, &z);

    temp = fix_div(z, _matrix_scale.gZ);
    temp2 = fix_div(x, _matrix_scale.gX);
    temp3 = fix_div(y, _matrix_scale.gY);

    temp3 = -fix_mul_div(temp3, _scrw, _scrh); // because projecting negates too, of course. Grrr.

    getpnt(point);
    point->gX = temp2;
    point->gY = temp3;
    point->gZ = temp;
    point->p3_flags = 0;

    return (point);
}

g3s_phandle g3_rotate_point(g3s_vector *v) {
    g3s_point *point;

    getpnt(point);
    xlate_rotate_point(v, &point->gX, &point->gY, &point->gZ);
    point->p3_flags = 0;

    code_point(point);
    return (point);
}

// matrix multiply and project a point. esi=vector, returns edi=point
g3s_phandle g3_transform_point(g3s_vector *v) {
    g3s_phandle tempH;

    // printf("g3_transform_point\n");

    tempH = g3_rotate_point(v);
    g3_project_point(tempH);
    return (tempH);
}

// takes edi = ptr to point. projects, fills in sx,sy, sets flag.
// returns 0 if z<=0, 1 if z>0.
// trashes eax,ecx,edx.
int g3_project_point(g3s_phandle p) {
    fix x, y, z, res;

#ifdef stereo_on
    test _g3d_stereo,
        1 jz no_stereo1
            // is this a sister point?
            cmp edi,
        _g3d_stereo_list jl not_sister

            // debug_brk 'yo, found projecting sister'

            // copy the point and add
            mov esi,
        edi sub esi, _g3d_stereo_base mov ecx,
        (size g3s_point) / 4 rep movsd

                               mov eax,
        _g3d_eyesep sub edi,
        (size g3s_point) // restore edi
        add[edi]
            .x,
        eax

            // call clip encoder on this point
            mov ecx,
        ebx call code_point mov ebx,
        ecx

            // project point like a normal point
            mov _g3d_stereo,
        0 pop esi call g3_project_point mov _g3d_stereo,
        1

        ret

            not_sister :
        // copy the point
        mov esi,
        edi add edi,
        _g3d_stereo_base mov ecx,
        (size g3s_point) / 4 rep movsd mov eax,
        _g3d_eyesep sub edi,
        (size g3s_point) // restore edi
        add[edi]
            .x,
        eax

            // call clip encoder
            mov ecx,
        ebx call code_point mov ebx,
        ecx

            sub edi,
        _g3d_stereo_base no_stereo1 :
#endif

        /*printf("g3_project_point\n");

        char printy[100];
        fix_sprint(printy, p->gX);
        printf("%s\n", printy);

        fix_sprint(printy, p->gY);
        printf("%s\n", printy);

        fix_sprint(printy, p->gZ);
        printf("%s\n", printy);*/

        // check if this point is in front of the back plane.
        z = p->gZ;
    if (z <= 0)
        return 0;
    x = p->gX;
    y = p->gY;

    // point is in front of back plane---do projection.
    // project y coordinate.
    res = fix_mul_div(y, _scrh, z);
    if (gOVResult) {
        p->codes |= CC_CLIP_OVERFLOW;
        return 1;
    }
    res = -res;
    if (AddLongWithOverflow(&res, res, _biasy)) {
        p->codes |= CC_CLIP_OVERFLOW;
        return 1;
    }
    p->sy = res;

    // now project x point
    res = fix_mul_div(x, _scrw, z);
    if (gOVResult) {
        p->codes |= CC_CLIP_OVERFLOW;
        return 1;
    }
    if (AddLongWithOverflow(&res, res, _biasx)) {
        p->codes |= CC_CLIP_OVERFLOW;
        return 1;
    }
    p->sx = res;

    // modify point flags to indicate projection.
    p->p3_flags |= PF_PROJECTED;

#ifdef stereo_on
    test _g3d_stereo, 1 jz no_stereo2 mov eax,
        [edi].sy // copy over old sy
            add edi,
        _g3d_stereo_base // load twin address
            mov[edi]
                .sy,
        eax // make new sy, could add the .5 addition here too

            mov eax,
        [edi].x
            // reproject the x coord
            imul _scrw                //* screen width
                proj_div_2 : idiv ecx /// z
                                 add eax,
        _biasx //+center
            mov[edi]
                .sx,
        eax // save

            // indicate projection
            or [edi].p3_flags,
        PF_PROJECTED

            // restore edi
            sub edi,
        _g3d_stereo_base no_stereo2 :
#endif

        // point has been projected.
        return 1;
}

// MLA - all the divide exception handler overflow stuff was removed, and
// checked before each divide.  So all of this stuf isn't needed
/*
        public  proj_div_0,proj_div_1,divide_overflow_3d
ifdef  stereo_on
        public  proj_div_2,divide_overflow_r3d
endif

//this gets called by the system divide overflow handler when there is an
//overflow at proj_div_0, proj_div_1
divide_overflow_3d:
//       fall    project_overflow
project_overflow:
        or      [edi].codes,CC_CLIP_OVERFLOW

ifdef stereo_on
        test    _g3d_stereo,1
        jz      no_stereo3
        add     edi,_g3d_stereo_base
divide_overflow_r3d:
        or      [edi].codes,CC_CLIP_OVERFLOW
        sub     edi,_g3d_stereo_base
no_stereo3:
endif

        cspew   "!"     //"project overflow!"
        // this did not use to restore this
        ex_set_div_action esi
        pop     esi
        ret
*/

// takes esi=ptr to array of vectors, edi=ptr to list for point handles,
// ecx=count
g3s_codes g3_transform_list(short n, g3s_phandle *dest_list, g3s_vector *v) {
    int i;
    g3s_phandle temphand;

    g_codes.or_ = 0;
    g_codes.and_ = 0xff;

    for (i = n; i > 0; i--) {
        temphand = g3_transform_point(v++);
        g_codes.or_ |= temphand->codes;
        g_codes.and_ &= temphand->codes;

        *(dest_list++) = temphand;
    }
    return (g_codes);
}

// takes esi=ptr to array of vectors, edi=ptr to list for point handles,
// ecx=count  returns bh=codes and, bl=codes or
g3s_codes g3_rotate_list(short n, g3s_phandle *dest_list, g3s_vector *v) {
    int i;
    g3s_phandle temphand;

    g_codes.or_ = 0;
    g_codes.and_ = 0xff;

    for (i = n; i > 0; i--) {
        temphand = g3_rotate_point(v++);
        g_codes.or_ |= temphand->codes;
        g_codes.and_ &= temphand->codes;

        *(dest_list++) = temphand;
    }
    return (g_codes);
}

// takes esi=ptr to array of point handles, ecx=count
g3s_codes g3_project_list(short n, g3s_phandle *point_list) {
    int i;
    g3s_phandle temphand;

    g_codes.or_ = 0;
    g_codes.and_ = 0xff;

    for (i = n; i > 0; i--) {
        temphand = *(point_list++);
        g_codes.or_ |= temphand->codes;
        g_codes.and_ &= temphand->codes;

        g3_project_point(temphand);
    }

    return (g_codes);
}

// takes esi=ptr to array of vectors, edi=ptr to dest vectors
g3s_phandle g3_rotate_light_norm(g3s_vector *v) {
    g3s_point *point;

    getpnt(point);
    do_rotate(v->gX, v->gY, v->gZ, &point->gX, &point->gY, &point->gZ);
    return (point);
}

// takes esi=ptr to normal vector. returns in <ecx,esi,eax>. trashes all regs
void rotate_norm(g3s_vector *v, fix *x, fix *y, fix *z) { do_norm_rotate(v->gX, v->gY, v->gZ, x, y, z); }

// does the rotate with the view matrix.
// takes <x,y,z> = <esi,edi,ebp>, returns <x,y,z> = <ecx,esi,eax>
void do_norm_rotate(fix x, fix y, fix z, fix *rx, fix *ry, fix *rz) {
    int64_t r;

    // this matrix multiply here will someday be optimized for zero and one terms
    // uses unscaled rotation matrix.

    // first column
    r = fix64_mul(x, uvm1) + fix64_mul(y, uvm4) + fix64_mul(z, uvm7);
    *rx = fix64_to_fix(r);

    // second column
    r = fix64_mul(x, uvm2) + fix64_mul(y, uvm5) + fix64_mul(z, uvm8);
    *ry = fix64_to_fix(r);

    // third column
    r = fix64_mul(x, uvm3) + fix64_mul(y, uvm6) + fix64_mul(z, uvm9);
    *rz = fix64_to_fix(r);
}

// made this a define - MLA
/*//takes esi=ptr to vector. returns <x,y,z> in <ecx,esi,eax>. trashes all regs
void xlate_rotate_point(g3s_vector *v, fix *x, fix *y, fix *z)
 {
        do_rotate(v->gX-_view_position.gX, v->gY-_view_position.gY,
v->gZ-_view_position.gZ,x,y,z);
 }*/

// does the rotate with the view matrix.
// takes <x,y,z> = <esi,edi,ebp>, returns <x,y,z> = <ecx,esi,eax>
void do_rotate(fix x, fix y, fix z, fix *rx, fix *ry, fix *rz) {
    // this matrix multiply here will someday be optimized for zero and one terms
    int64_t r;
    // first column
    r = fix64_mul(x, vm1) + fix64_mul(y, vm4) + fix64_mul(z, vm7);
    *rx = fix64_to_fix(r);

    // second column
    r = fix64_mul(x, vm2) + fix64_mul(y, vm5) + fix64_mul(z, vm8);
    *ry = fix64_to_fix(r);

    // third column
    r = fix64_mul(x, vm3) + fix64_mul(y, vm6) + fix64_mul(z, vm9);
    *rz = fix64_to_fix(r);
}

// rotate an x delta. takes edi=dest vector, eax=dx
// trashes eax,ebx,edx
void g3_rotate_delta_x(g3s_vector *dest, fix dx) {
    dest->gX = fix_mul(dx, vm1);
    dest->gY = fix_mul(dx, vm2);
    dest->gZ = fix_mul(dx, vm3);
}

// rotate a y delta. takes edi=dest vector, eax=dy
// trashes eax,ebx,edx
void g3_rotate_delta_y(g3s_vector *dest, fix dy) {
    dest->gX = fix_mul(dy, vm4);
    dest->gY = fix_mul(dy, vm5);
    dest->gZ = fix_mul(dy, vm6);
}

// rotate a z delta. takes edi=dest vector, eax=dz
// trashes eax,ebx,edx
void g3_rotate_delta_z(g3s_vector *dest, fix dz) {
    dest->gX = fix_mul(dz, vm7);
    dest->gY = fix_mul(dz, vm8);
    dest->gZ = fix_mul(dz, vm9);
}

// rotate an xz delta. takes edi=dest vector, eax=dx, ebx=dz
// trashes eax,ebx,edx
void g3_rotate_delta_xz(g3s_vector *dest, fix dx, fix dz) {
    int64_t r;

    // first column
    r = fix64_mul(dx, vm1) + fix64_mul(dz, vm7);
    dest->gX = fix64_to_fix(r);

    // second column
    r = fix64_mul(dx, vm2) + fix64_mul(dz, vm8);
    dest->gY = fix64_to_fix(r);

    // third column
    r = fix64_mul(dx, vm3) + fix64_mul(dz, vm9);
    dest->gZ = fix64_to_fix(r);
}

// rotate an xy delta. takes edi=dest vector, eax=dx, ebx=dy
// trashes eax,ebx,edx
void g3_rotate_delta_xy(g3s_vector *dest, fix dx, fix dy) {
    int64_t r;

    // first column
    r = fix64_mul(dx, vm1) + fix64_mul(dy, vm4);
    dest->gX = fix64_to_fix(r);

    // second column
    r = fix64_mul(dx, vm2) + fix64_mul(dy, vm5);
    dest->gY = fix64_to_fix(r);

    // third column
    r = fix64_mul(dx, vm3) + fix64_mul(dy, vm6);
    dest->gZ = fix64_to_fix(r);
}

// rotate a yz delta. takes edi=dest vector, eax=dy, ebx=dz
// trashes eax,ebx,edx
void g3_rotate_delta_yz(g3s_vector *dest, fix dy, fix dz) {
    int64_t r;

    // first column
    r = fix64_mul(dy, vm4) + fix64_mul(dz, vm7);
    dest->gX = fix64_to_fix(r);

    // second column
    r = fix64_mul(dy, vm5) + fix64_mul(dz, vm8);
    dest->gY = fix64_to_fix(r);

    // third column
    r = fix64_mul(dy, vm6) + fix64_mul(dz, vm9);
    dest->gZ = fix64_to_fix(r);
}

// rotate a delta vector. takes edi=dest, eax,ebx,ecx=dx,dy,dz
// trashes all but ebp,edi
void g3_rotate_delta_xyz(g3s_vector *dest, fix dx, fix dy, fix dz) {
    do_rotate(dx, dy, dz, &dest->gX, &dest->gY, &dest->gZ);
}

// rotate a delta vector. takes edi=dest, esi=src
// trashes all but ebp,edi
void g3_rotate_delta_v(g3s_vector *dest, g3s_vector *src) {
    do_rotate(src->gX, src->gY, src->gZ, &dest->gX, &dest->gY, &dest->gZ);
}

// like add_delta, but creates and returns a new point
// takes esi=src, ebx=delta, returns edi=new point
// trashes eax,ebx
g3s_phandle g3_copy_add_delta_v(g3s_phandle src, g3s_vector *delta) {
    g3s_point *point;

    getpnt(point);
    point->gX = src->gX + delta->gX;
    point->gY = src->gY + delta->gY;
    point->gZ = src->gZ + delta->gZ;
    point->p3_flags = 0;
    code_point(point);
    return (point);
}

// adds a delta vector (created by rotate delta) to a point
// takes edi=point, esi=delta. clears projected bit, computes codes
// trashes eax,esi,bl
void g3_add_delta_v(g3s_phandle p, g3s_vector *delta) {
    p->gX += delta->gX;
    p->gY += delta->gY;
    p->gZ += delta->gZ;

    p->p3_flags &= ~PF_PROJECTED;
    code_point(p);
}

// add an x delta to a point. takes edi=point, eax=dx
// trashes eax,ebx,edx
void g3_add_delta_x(g3s_phandle p, fix dx) {
    p->gX += fix_mul(vm1, dx);
    p->gY += fix_mul(vm2, dx);
    p->gZ += fix_mul(vm3, dx);
    p->p3_flags &= ~PF_PROJECTED;

    code_point(p);
}

// add a y delta to a point. takes edi=point, eax=dy
// trashes eax,ebx,edx
void g3_add_delta_y(g3s_phandle p, fix dy) {
    p->gX += fix_mul(vm4, dy);
    p->gY += fix_mul(vm5, dy);
    p->gZ += fix_mul(vm6, dy);
    p->p3_flags &= ~PF_PROJECTED;

    code_point(p);
}

// add a z delta to a point. takes edi=point, eax=dz
// trashes eax,ebx,edx
void g3_add_delta_z(g3s_phandle p, fix dz) {
    p->gX += fix_mul(vm7, dz);
    p->gY += fix_mul(vm8, dz);
    p->gZ += fix_mul(vm9, dz);
    p->p3_flags &= ~PF_PROJECTED;

    code_point(p);
}

// add an xy delta to a point. takes edi=point, eax=dx, ebx=dy
// trashes eax,ebx,ecx,edx,esi
void g3_add_delta_xy(g3s_phandle p, fix dx, fix dy) {
    int64_t r;

    // first column
    r = fix64_mul(dx, vm1) + fix64_mul(dy, vm4);
    p->gX += fix64_to_fix(r);

    // second column
    r = fix64_mul(dx, vm2) + fix64_mul(dy, vm5);
    p->gY += fix64_to_fix(r);

    // third column
    r = fix64_mul(dx, vm3) + fix64_mul(dy, vm6);
    p->gZ += fix64_to_fix(r);

    p->p3_flags &= ~PF_PROJECTED;
    code_point(p);
}

// add an xz delta to a point. takes edi=point, eax=dx, ebx=dz
// trashes eax,ebx,ecx,edx,esi
void g3_add_delta_xz(g3s_phandle p, fix dx, fix dz) {
    int64_t r;

    // first column
    r = fix64_mul(dx, vm1) + fix64_mul(dz, vm7);
    p->gX += fix64_to_fix(r);

    // second column
    r = fix64_mul(dx, vm2) + fix64_mul(dz, vm8);
    p->gY += fix64_to_fix(r);

    // third column
    r = fix64_mul(dx, vm3) + fix64_mul(dz, vm9);
    p->gZ += fix64_to_fix(r);

    p->p3_flags &= ~PF_PROJECTED;
    code_point(p);
}

// add an yz delta to a point. takes edi=point, eax=dy, ebx=dz
// trashes eax,ebx,ecx,edx,esi
void g3_add_delta_yz(g3s_phandle p, fix dy, fix dz) {
    int64_t r;

    // first column
    r = fix64_mul(dy, vm4) + fix64_mul(dz, vm7);
    p->gX += fix64_to_fix(r);

    // second column
    r = fix64_mul(dy, vm5) + fix64_mul(dz, vm8);
    p->gY += fix64_to_fix(r);

    // third column
    r = fix64_mul(dy, vm6) + fix64_mul(dz, vm9);
    p->gZ += fix64_to_fix(r);

    p->p3_flags &= ~PF_PROJECTED;
    code_point(p);
}

// add an xyz delta to a point. takes edi=point, eax=dx, ebx=dy, ecx=dz
// trashes eax,ebx,ecx,edx,esi
void g3_add_delta_xyz(g3s_phandle p, fix dx, fix dy, fix dz) {
    fix rx, ry, rz;

    do_rotate(dx, dy, dz, &rx, &ry, &rz);

    p->gX += rx;
    p->gY += ry;
    p->gZ += rz;

    p->p3_flags &= ~PF_PROJECTED;
    code_point(p);
}

// like add_delta, but creates and returns a new point in edi
// add an x delta to a point. takes esi=point, eax=dx
// trashes eax,ebx,edx
g3s_phandle g3_copy_add_delta_x(g3s_phandle src, fix dx) {
    g3s_point *point;

    getpnt(point);
    point->gX = src->gX + fix_mul(dx, vm1);
    point->gY = src->gY + fix_mul(dx, vm2);
    point->gZ = src->gZ + fix_mul(dx, vm3);
    point->p3_flags = 0;
    code_point(point);
    return (point);
}

// like add_delta, but creates and returns a new point in edi
// add a y delta to a point. takes esi=point, eax=dy
// trashes eax,ebx,edx
g3s_phandle g3_copy_add_delta_y(g3s_phandle src, fix dy) {
    g3s_point *point;

    getpnt(point);
    point->gX = src->gX + fix_mul(dy, vm4);
    point->gY = src->gY + fix_mul(dy, vm5);
    point->gZ = src->gZ + fix_mul(dy, vm6);
    point->p3_flags = 0;
    code_point(point);
    return (point);
}

// like add_delta, but creates and returns a new point in edi
// add a z delta to a point. takes esi=point, eax=dz
// trashes eax,ebx,edx
g3s_phandle g3_copy_add_delta_z(g3s_phandle src, fix dz) {
    g3s_point *point;

    getpnt(point);
    point->gX = src->gX + fix_mul(dz, vm7);
    point->gY = src->gY + fix_mul(dz, vm8);
    point->gZ = src->gZ + fix_mul(dz, vm9);
    point->p3_flags = 0;
    code_point(point);
    return (point);
}

// like add_delta, but modifies an existing point in edi
// add an x delta to a point. takes esi=src point, edi=replace point, ax=dx
// trashes eax,ebx,edx
g3s_phandle g3_replace_add_delta_x(g3s_phandle src, g3s_phandle dst, fix dx) {
    dst->gX = src->gX + fix_mul(dx, vm1);
    dst->gY = src->gY + fix_mul(dx, vm2);
    dst->gZ = src->gZ + fix_mul(dx, vm3);
    dst->p3_flags = 0;
    code_point(dst);
    return (dst);
}

// like add_delta, but modifies an existing point in edi
// add a y delta to a point. takes esi=src point, edi=replace point, ax=dy
// trashes eax,ebx,edx
g3s_phandle g3_replace_add_delta_y(g3s_phandle src, g3s_phandle dst, fix dy) {
    dst->gX = src->gX + fix_mul(dy, vm4);
    dst->gY = src->gY + fix_mul(dy, vm5);
    dst->gZ = src->gZ + fix_mul(dy, vm6);
    dst->p3_flags = 0;
    code_point(dst);
    return (dst);
}

// like add_delta, but modifies an existing point in edi
// add a z delta to a point. takes esi=src point, edi=replace point, ax=dz
// trashes eax,ebx,edx
g3s_phandle g3_replace_add_delta_z(g3s_phandle src, g3s_phandle dst, fix dz) {
    dst->gX = src->gX + fix_mul(dz, vm7);
    dst->gY = src->gY + fix_mul(dz, vm8);
    dst->gZ = src->gZ + fix_mul(dz, vm9);
    dst->p3_flags = 0;
    code_point(dst);
    return (dst);
}

// like add_delta, but creates and returns a new point in edi
// add an xy delta to a point. takes edi=point, eax=dx, ebx=dy
// trashes eax,ebx,ecx,edx,esi
g3s_phandle g3_copy_add_delta_xy(g3s_phandle src, fix dx, fix dy) {
    g3s_point *point;
    int64_t r;

    getpnt(point);

    // first column
    r = fix64_mul(dx, vm1) + fix64_mul(dy, vm4);
    point->gX = src->gX + fix64_to_fix(r);

    // second column
    r = fix64_mul(dx, vm2) + fix64_mul(dy, vm5);
    point->gY = src->gY + fix64_to_fix(r);

    // third column
    r = fix64_mul(dx, vm3) + fix64_mul(dy, vm6);
    point->gZ = src->gZ + fix64_to_fix(r);

    point->p3_flags = 0;
    code_point(point);
    return (point);
}

// like add_delta, but creates and returns a new point in edi
// add an xz delta to a point. takes edi=point, eax=dx, ebx=dz
// trashes eax,ebx,ecx,edx,esi
g3s_phandle g3_copy_add_delta_xz(g3s_phandle src, fix dx, fix dz) {
    g3s_point *point;
    int64_t r;

    getpnt(point);

    // first column
    r = fix64_mul(dx, vm1) + fix64_mul(dz, vm7);
    point->gX = src->gX + fix64_to_fix(r);

    // second column
    r = fix64_mul(dx, vm2) + fix64_mul(dz, vm8);
    point->gY = src->gY + fix64_to_fix(r);

    // third column
    r = fix64_mul(dx, vm3) + fix64_mul(dz, vm9);
    point->gZ = src->gZ + fix64_to_fix(r);

    point->p3_flags = 0;
    code_point(point);
    return (point);
}

// like add_delta, but creates and returns a new point in edi
// add an yz delta to a point. takes edi=point, eax=dy, ebx=dz
// trashes eax,ebx,ecx,edx,esi
g3s_phandle g3_copy_add_delta_yz(g3s_phandle src, fix dy, fix dz) {
    g3s_point *point;
    int64_t r;

    getpnt(point);

    // first column
    r = fix64_mul(dy, vm4) + fix64_mul(dz, vm7);
    point->gX = src->gX + fix64_to_fix(r);

    // second column
    r = fix64_mul(dy, vm5) + fix64_mul(dz, vm8);
    point->gY = src->gY + fix64_to_fix(r);

    // third column
    r = fix64_mul(dy, vm6) + fix64_mul(dz, vm9);
    point->gZ = src->gZ + fix64_to_fix(r);

    point->p3_flags = 0;
    code_point(point);
    return (point);
}

// like add_delta, but creates and returns a new point in edi
// add an xyz delta to a point. takes edi=point, eax=dx, ebx=dy, ecx=dz
// trashes eax,ebx,ecx,edx,esi
g3s_phandle g3_copy_add_delta_xyz(g3s_phandle src, fix dx, fix dy, fix dz) {
    g3s_point *point;

    getpnt(point);
    do_rotate(dx, dy, dz, &point->gX, &point->gY, &point->gZ);

    point->gX += src->gX;
    point->gY += src->gY;
    point->gZ += src->gZ;

    point->p3_flags = 0;
    code_point(point);
    return (point);
}
