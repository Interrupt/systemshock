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
 * $Header: n:/project/lib/src/edms/RCS/edms_int.h 1.4 1993/05/13 15:46:06 roadkill Exp $
 */

//	This header file contains the codes for the object types and functions in Soliton.
//	=====================================================================

#ifndef __EDMS_INT_H
#define __EDMS_INT_H

//	Are we building a shipping version?
//	===================================
//#define EDMS_SHIPPABLE	1

//	Things like Getch()
//	-------------------
////#include <conio.h>

//#pragma INLINE_DEPTH 255
//#pragma INLINE_RECURSION ON

//	Cool math universe...
//	---------------------
#include "fixpp.h"

// Physics handle typedef
// ======================
#include "physhand.h"

#include "ss_flet.h"

//	Actual object types...
//	======================
extern Q VACUUM, MARBLE, ROBOT, FIELD_POINT, BIPED, PELVIS, DEATH, D_FRAME;

//	Commands for soliton from the state stream (in the Global.cc file)...
//	=====================================================================
extern Q END;

//	Max and Minima...
//	=================
#define MAX_OBJ 96
//#define DOF_MAX 96
#define DOF_MAX 40
#define EDMS_DATA_SIZE 100

#define DOF 7        // degrees of freedom
#define DOF_DERIVS 4 // d/dt each dof this-1 times

// Dan, these are model specific, so we need more general names than below.  I suggest
//#define DOF_X         0
//#define DOF_Y         1
//#define DOF_Z         2
//#define DOF_ORIENT_0  3
//#define DOF_ORIENT_1  4
//#define DOF_ORIENT_2  5
//#define DOF_ORIENT_3  6
// But I can't spend time right now changing the references in your code ;^) ...

#define DOF_X 0
#define DOF_Y 1
#define DOF_Z 2
#define DOF_ALPHA 3
#define DOF_BETA 4
#define DOF_GAMMA 5
#define DOF_DIRAC 6

//	Memory conserving stuff...
//	==========================
typedef Q EDMS_Argument_Block[MAX_OBJ][DOF][DOF_DERIVS];
typedef Q (*EDMS_Argblock_Pointer)[DOF][DOF_DERIVS];

//	Have some functions...
//	======================

//	General functions...
//	--------------------
typedef struct {
    fix playfield_size;
    int32_t min_physics_handle;
    void (*collision_callback)(physics_handle caller, physics_handle victim, int32_t badness, int32_t DATA1,
                               int32_t DATA2, fix location[3]),
        (*autodestruct_callback)(physics_handle caller), (*awol_callback)(physics_handle caller),
        (*snooz_callback)(physics_handle caller);
    void *argblock_pointer;
} EDMS_data;

//	Structs...
//	==========
typedef struct {
    fix X, Y, Z, alpha, beta, gamma;
    fix X_dot, Y_dot, Z_dot, alpha_dot, beta_dot, gamma_dot;
} State;

//	Stuff that used to be in physhand.h....
//	=======================================
typedef int32_t object_number;

#define physics_handle_to_object_number(ph) (ph2on[ph])
#define object_number_to_physics_handle(on) (on2ph[on])

extern "C" {

void EDMS_init_handles(void);
physics_handle EDMS_bind_object_number(object_number on);
void EDMS_remap_object_number(object_number old, object_number nu);
physics_handle EDMS_get_free_ph(void);
void EDMS_release_object(physics_handle ph);

}

//	Terrain
//	=======
Q terrain(Q X, Q Y, int32_t deriv);                         // This calls Terrain()
TerrainHit indoor_terrain(Q X, Q Y, Q Z, Q R, physics_handle ph, TFType type); // Indoor for Citadel, FBO, etc...

extern "C" {

fix Terrain(fix X, fix Y, int32_t deriv);                           // This is provided by the user...
TerrainHit Indoor_Terrain(fix X, fix Y, fix Z, fix R, physics_handle ph, TFType type); // As is this...

//	Here's the actual indoor guy we ask for...
//	------------------------------------------
typedef struct {
    // Filled by user when Indoor_Terrain is called...
    fix cx, cy, cz;
    fix fx, fy, fz;
    fix wx, wy, wz;
} TerrainData;

extern TerrainData terrain_info; // Struct name EDMS expects...

// Freefall terrain data structures...
// -----------------------------------
typedef struct {
    // The ground...
    fix g_height, g_dx, g_dy, g_dz;
    // Any walls...
    fix w_x, w_y, w_z;
    // Squishiness, friction, et cetera...
    fix terrain_information;
    // For terrain return information...
    int32_t DATA1, DATA2;
    // Only needed for "fast" terrain calls
    fix my_size;
    // Who's responsible...
    physics_handle caller;
} terrain_ff;

bool FF_terrain(fix X, fix Y, fix Z, uchar fast, terrain_ff *TFF); // From Freefall...
bool FF_raycast(fix x, fix y, fix z, fix *vec, fix range, fix *where_hit, terrain_ff *tff);
}

bool ff_terrain(Q X, Q Y, Q Z, uchar fast, terrain_ff *TFF); // For the refined...
bool ff_raycast(Q x, Q y, Q z, Fixpoint *vec, Q range, Fixpoint *where_hit, terrain_ff *FFT);

//		Motion package functions...
//		===========================

//		Marble...
//		---------
void marble_X(int32_t object);
void marble_Y(int32_t object);
void marble_Z(int32_t object);

//		Robot...
//		--------
void robot_X(int32_t object);
void robot_Y(int32_t object);
void robot_Z(int32_t object);

//		Deformable objects...
//		---------------------
void field_point_X(int32_t object);
void field_point_Y(int32_t object);
void field_point_Z(int32_t object);

// Have some arrays...
// ===================

// binary database (collision) operators...
// ========================================

// Playfield information and scaling...
// ------------------------------------
//#define COLLISION_SIZE 100
#define DELTA_BY_TWO .5

#define NUM_OBJECT_BITS 32

#define object_bit(n) (1 << (n & 31))

// To turn on an element...
// ------------------------
#define write_object_bit(X, Y, obit) (data[X][Y] |= obit)

// Turn it off...
// --------------
#define delete_object_bit(X, Y, obit) (data[X][Y] &= ~(obit))

// Test a bit...
// -------------
#define test_object_bit(X, Y, object) (data[X][Y] & object_bit(object))

// Check for a given collision...
// ------------------------------
#define check_object(caller, looker)                                                                  \
    (data[(hash_scale * A[caller][DOF_X][0]).to_int()][(hash_scale * A[caller][DOF_Y][0]).to_int()] & \
     object_bit(looker))

// This used to be a function in collide.cc
// I had to change the name because Seamus had some files locked out.
#define check_for_hit(other_object) (test_bitmask & object_bit(other_object))

//	Ta Daa.
//	=======

#include "externs.h"
#endif // __EDMS_INT_H
