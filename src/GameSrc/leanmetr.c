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
 * $Source: r:/prj/cit/src/RCS/leanmetr.c $
 * $Revision: 1.31 $
 * $Author: mahk $
 * $Date: 1994/11/09 20:45:40 $
 *
 */

#include "faketime.h"
#include "citres.h"
#include "physics.h"
#include "player.h"
#include "criterr.h"
#include "froslew.h"
#include "objprop.h"
#include "objsim.h"
#include "tools.h"
#include "wares.h"
#include "canvchek.h"

#include "gamescr.h"
#include "otrip.h"

#include "gr2ss.h"

// -------
// DEFINES
// -------


#define SLOT_EYEMETER_X 141
#define SLOT_EYEMETER_Y 1

#define FULL_EYEMETER_X 10
#define FULL_EYEMETER_Y 1

#define EYEMETER_X() (current_meter_region->abs_x)
#define EYEMETER_Y() (current_meter_region->abs_y)
#define LEANOMETER_XOFF 21
#define LEANOMETER_YOFF 0
#define LEANOMETER_X() (EYEMETER_X() + LEANOMETER_XOFF)
#define LEANOMETER_Y() (EYEMETER_Y() + LEANOMETER_YOFF)

#define EYEMETER_W 19
#define EYEMETER_H 22
#define EYE_LEFT_MARGIN 2
#define DISCRETE_EYE_POSITIONS 3
#define DISCRETE_MIDDLE_H 6

#define MAX_EYE_ANGLE (8 * FIXANG_PI / 18)

#define LEANOMETER_W 23
#define LEANOMETER_H EYEMETER_H

#define NUM_LEAN_BMAPS 9
#define BMAPS_PER_POSTURE 3

extern uchar full_game_3d;

static ubyte discrete_eye_height[DISCRETE_EYE_POSITIONS] = {
    3,
    10,
    18,
};
ubyte hires_eye_height[DISCRETE_EYE_POSITIONS] = {
    8,
    26,
    45,
};
LGPoint shield_offsets[9] = {
    {2, 1}, // stand right
    {2, 1}, // stand
    {2, 1}, // stand left
    {2, 1}, // crouch right
    {2, 1}, // crouch
    {2, 0}, // crouch left
    {2, 1}, // prone right
    {2, 1}, // prone
    {0, 1}  // prone left
};

void EDMS_lean_o_meter(physics_handle ph, fix *lean, fix *crouch);

// -------
// GLOBALS
// -------
extern uchar gBioInited;

LGRegion slot_meter_region;
LGRegion fullscrn_meter_region;
LGRegion *current_meter_region = &slot_meter_region;

#define PICK_METER_REGION(full) ((full) ? &fullscrn_meter_region : &slot_meter_region)

ushort lean_bmap_res = 0;
ushort shield_bmap_res = 0;

#define lean_bmap_id(i) (MKREF(lean_bmap_res, i))
#define eye_bmap_r(i) (get_bitmap_from_ref(REF_IMG_bmEyeIconR + (i)))
#define eye_bmap_l(i) (get_bitmap_from_ref(REF_IMG_bmEyeIconL + (i)))
// KLC - lock instead of get #define lean_bmaps(i) (get_bitmap_from_ref(lean_bmap_id(i)))
#define lean_bmaps(i) (lock_bitmap_from_ref(lean_bmap_id(i)))
#define meter_bkgnd() (get_bitmap_from_ref(full_game_3d ? REF_IMG_bmLeanBkgndTransp : REF_IMG_bmLeanBkgnd))

uchar eye_fine_mode = FALSE;


// ---------
// INTERNALS
// ---------
#define MAX_LEAN_BASE (RES_leanMeterRad - RES_leanMeterBase)

void set_base_lean_bmap(uchar shield) {
    int v;
    ushort baseRes;

    // Set the lean bitmaps resource.

    v = player_struct.hardwarez[CPTRIP(ENV_HARD_TRIPLE)]; // v will be 0-2.
    if (v > MAX_LEAN_BASE)
        v = MAX_LEAN_BASE;
    baseRes = RES_leanMeterBase + v;

    if (baseRes != lean_bmap_res) // If the base lean bitmap has changed:
    {
        if (lean_bmap_res != 0) // free the old bitmap
        {
            ResUnlock(lean_bmap_res);
            ResDrop(lean_bmap_res);
        }
        ResLock(baseRes); // Load hi and lock the new bitmap series.
        lean_bmap_res = baseRes; // this is our base bitmap now
    }

    // Set the shield bitmaps resource.

    if (shield) {
        v = SHIELD_SETTING(player_struct.hardwarez_status[CPTRIP(SHIELD_HARD_TRIPLE)]);
        if (player_struct.hardwarez[CPTRIP(SHIELD_HARD_TRIPLE)] == SHIELD_VERSIONS)
            v = SHIELD_VERSIONS - 1;
        baseRes = RES_leanShield1 + v;

        if (baseRes != shield_bmap_res) {
            // If the base shield bitmap has changed:
            if (shield_bmap_res != 0) {
                // free the old bitmap
                ResUnlock(shield_bmap_res);
                ResDrop(shield_bmap_res);
            }
            ResLock(baseRes); // Load hi and lock the new bitmap series.
            shield_bmap_res = baseRes; // this is our base shield bitmap now
        }
    } else {
        // If shields are now off:
        if (shield_bmap_res != 0) {
            // If shields were previously on, free up the shield bitmaps.
            ResUnlock(shield_bmap_res);
            ResDrop(shield_bmap_res);
            shield_bmap_res = 0;
        }
    }
}

#define LEAN_CONST (fix_2pi / 10)
#define LEAN_TO_LEANX(ln) lg_min(100, lg_max(-100, (100 * (ln) / LEAN_CONST)))

#define PLAYER_HGT (fix_make(0, 0xbd00))
#define CROUCH_CONST (2 * PLAYER_HGT / 5)
#define CROUCH_TO_POSTURE(c) lg_min(2, lg_max(0, (2 - lg_max(c, 0) * 2 / CROUCH_CONST)))

// The posture meter output is filtered by a capacitor,
// the constant below is the reciprocal of the
// filter's RC time constant, in units of 1/sec.
#define POSTURE_FILTER_RATE 20

#ifdef BIASED_CAPACITOR
fix compute_filter_weight(ulong deltat) {
    int rate = POSTURE_FILTER_RATE;
    fix bias = FIX_UNIT * CONTROL_MAX_VAL / (CONTROL_MAX_VAL + 3 * abs(player_struct.controls[CONTROL_YVEL]));
    fix weight = fix_make(deltat, 0) * rate >> APPROX_CIT_CYCLE_SHFT;
    Spew(DSRC_USER_I_Lean, ("posture filter weight = %q, bias = %q\n", weight, bias));
    weight = fix_mul(weight, bias);
    return weight;
}
#else
fix compute_filter_weight(ulong deltat) {
    int rate = POSTURE_FILTER_RATE;
    fix weight = fix_make(deltat, 0) * rate >> APPROX_CIT_CYCLE_SHFT;

    return weight;
}
#endif

fix apply_weighted_filter(fix input, fix state, ulong deltat) {
    fix weight = compute_filter_weight(deltat);
    return fix_div(fix_mul(weight, input) + state, weight + FIX_UNIT);
}

#define INTENDED_HGT (PLAYER_HGT * (NUM_POSTURES - player_struct.posture - 1) / (NUM_POSTURES - 1))

#define STORE_STATE(fval) \
    (player_struct.lean_filter_state = (ushort)((fval) > FIX_UNIT ? FIX_UNIT - 1 : fix_frac(fval)))
#define GET_STATE (fix_make(0, player_struct.lean_filter_state));

#define SLAM_DURATION (CIT_CYCLE / 2)

void slam_posture_meter_state(void) {
    STORE_STATE(INTENDED_HGT);
    player_struct.posture_slam_state = player_struct.game_time + SLAM_DURATION;
}

fix velocity_crouch_filter(fix crouch) {
    ubyte posture = player_struct.posture + 1;
    fix hgt = INTENDED_HGT;
    fix vel = posture * FIX_UNIT *
              (abs(player_struct.controls[CONTROL_YVEL] + abs(player_struct.controls[CONTROL_ZVEL]))) /
              (2 * CONTROL_MAX_VAL);
    return fix_div(fix_mul(hgt, vel) + crouch, vel + FIX_UNIT);
}

void lean_icon(LGPoint *pos, grs_bitmap **icon, int *inum) {
    int posture, leanx;
    uchar *bp;

    // Determine the posture and lean amount for the player currently.
    if (!global_fullmap->cyber) {
        fix ln, crouch;
        fix state = GET_STATE;

        EDMS_lean_o_meter(PLAYER_PHYSICS, &ln, &crouch);

        crouch = velocity_crouch_filter(crouch);
        if (player_struct.game_time > player_struct.posture_slam_state)
            state = apply_weighted_filter(crouch, state, player_struct.deltat);
        STORE_STATE(state);
        posture = CROUCH_TO_POSTURE(state);
        leanx = LEAN_TO_LEANX(ln);
    } else {
        posture = POSTURE_STAND;
        leanx = 0;
    }

    // Calculate the bitmap resource index based on posture and lean.
    *inum = posture * BMAPS_PER_POSTURE + ((100 - leanx) * BMAPS_PER_POSTURE / 201);

    // Get a pointer to the corresponding lean bitmap.
    FrameDesc *f = RefGet(MKREF(lean_bmap_res,*inum));
    if (f != NULL) {
        f->bm.bits = (uchar *)(f + 1);
        *icon = &(f->bm);
    } else
        DEBUG("%s: No lean resource bitmap!", __FUNCTION__);

    // Determine where to draw the bitmap.
    pos->y = 53 - (*icon)->h;
    pos->x = (46 - (*icon)->w + 1) * abs(leanx) / 600;
    if (leanx < 0)
        pos->x = -pos->x;
    pos->x += LEANOMETER_X() + (25) / 2 - ((*icon)->w + 1) / 2;
    pos->y += LEANOMETER_Y() - 31;
}

static void undraw_meter_area(LGRect *r) {
    short a, b, c, d;
    char saveMode;
    int x, y;

    STORE_CLIP(a, b, c, d);
    ss_safe_set_cliprect(r->ul.x, r->ul.y, r->lr.x, r->lr.y);
    x = EYEMETER_X();
    y = EYEMETER_Y();

    if (is_onscreen())
        uiHideMouse(r);
    ss_bitmap(meter_bkgnd(), x, y);
    if (is_onscreen())
        uiShowMouse(r);

    RESTORE_CLIP(a, b, c, d);
}

void player_reset_eye(void) { player_struct.eye_pos = eye_mods[1] = 0; }

void player_set_eye(byte eyecntl) {
    int theta = MAX_EYE_ANGLE * eyecntl / CONTROL_MAX_VAL;
    player_struct.eye_pos = theta;

    if (theta < 0)
        theta += 2 * FIXANG_PI;
    eye_mods[1] = theta;
}

byte player_get_eye(void) {
    int theta = eye_mods[1];
    if (theta > FIXANG_PI)
        theta -= 2 * FIXANG_PI;
    return (byte)(theta * CONTROL_MAX_VAL / MAX_EYE_ANGLE);
}

void player_set_eye_fixang(int ang) {
    int theta = ang;
    if (abs(ang) > MAX_EYE_ANGLE)
        theta = (ang < 0) ? -MAX_EYE_ANGLE : MAX_EYE_ANGLE;
    player_struct.eye_pos = theta;
    if (theta < 0)
        theta += 2 * FIXANG_PI;
    eye_mods[1] = theta;
}

int player_get_eye_fixang(void) {
    int theta = eye_mods[1];
    if (theta > FIXANG_PI)
        theta -= 2 * FIXANG_PI;
    return theta;
}

uchar eye_mouse_handler(uiEvent *ev, LGRegion *r, intptr_t data) {
    short x = ev->pos.x - r->abs_x;
    short y = ev->pos.y - r->abs_y;
    extern uchar hack_takeover;
    if (hack_takeover || global_fullmap->cyber)
        return FALSE;
    if (x < 0 || x >= EYEMETER_W)
        return FALSE;
    eye_fine_mode = 2 * x > EYEMETER_W;
    if (!eye_fine_mode)
        y = discrete_eye_height[y * DISCRETE_EYE_POSITIONS / EYEMETER_H];
    if (ev->mouse_data.buttons & (1 << MOUSE_LBUTTON)) {
        int theta;
        if ((ev->mouse_data.action & MOUSE_LDOWN) == 0 && uiLastMouseRegion[MOUSE_LBUTTON] != r)
            return FALSE;
        if (eye_fine_mode)
            theta = -2 * MAX_EYE_ANGLE * (y) / (EYEMETER_H - 1) + MAX_EYE_ANGLE;
        else
            theta = -FIXANG_PI / 6 * (y * DISCRETE_EYE_POSITIONS / EYEMETER_H - 1);
        //      ui_mouse_constrain_xy(me->pos.x,r->abs_y,me->pos.x,r->abs_y+EYEMETER_H-1);
        player_set_eye_fixang(theta);
        physics_set_relax(CONTROL_YZROT, FALSE);
    }
    if (ev->mouse_data.buttons == 0) {
        //      mouse_constrain_xy(0,0,grd_cap->w-1,grd_cap->h-1);
    }
    return TRUE;
}

uchar lean_mouse_handler(uiEvent *ev, LGRegion *r, intptr_t data) {
    short x = ev->pos.x - r->abs_x - LEANOMETER_XOFF;
    short y = ev->pos.y - r->abs_y - LEANOMETER_YOFF;
    if (x < 0 || x >= LEANOMETER_W || global_fullmap->cyber)
        return FALSE;
    if (ev->mouse_data.buttons & (1 << MOUSE_LBUTTON)) {
        short posture = y * 3 / LEANOMETER_H;
        short xlean = x * 220 / (LEANOMETER_W - 1) - 110;
        if ((ev->mouse_data.action & MOUSE_LDOWN) == 0 && uiLastMouseRegion[MOUSE_LBUTTON] != r)
            return FALSE;
        if (xlean > 10)
            xlean -= 10;
        else if (xlean < -10)
            xlean += 10;
        else
            xlean = 0;
        if (posture != player_struct.posture)
            player_set_posture(posture);
        player_set_lean(xlean, player_struct.leany);
        physics_set_relax(CONTROL_XZROT, FALSE);
        //     ui_mouse_constrain_xy(LEANOMETER_X(),LEANOMETER_Y()+posture*LEANOMETER_H/3+1,LEANOMETER_X()+LEANOMETER_W-1,LEANOMETER_Y()+(posture+1)*LEANOMETER_H/3-1);
    }
    if (ev->mouse_data.buttons == 0) {
        //      mouse_unconstrain();
    }
    return TRUE;
}

    // ---------
    // EXTERNALS
    // ---------

#define BAD_REGION_CRITERR 3000

void init_posture_meters(LGRegion *root, uchar fullscreen) {
    LGRegion *reg = PICK_METER_REGION(fullscreen);
    int id;
    LGRect r = {{0, 0}, {LEANOMETER_W + LEANOMETER_XOFF, EYEMETER_H}};
    errtype err;

    if (fullscreen) {
        RECT_MOVE(&r, MakePoint(FULL_EYEMETER_X, FULL_EYEMETER_Y));
    } else {
        RECT_MOVE(&r, MakePoint(SLOT_EYEMETER_X, SLOT_EYEMETER_Y));
    }
    err = region_create(root, reg, &r, 0, 0, REG_USER_CONTROLLED | AUTODESTROY_FLAG, NULL, NULL, NULL, NULL);
    if (err != OK)
        critical_error(BAD_REGION_CRITERR);
    uiInstallRegionHandler(reg, UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE, eye_mouse_handler, 0, &id);
    uiInstallRegionHandler(reg, UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE, lean_mouse_handler, 0, &id);
}

void update_lean_meter(uchar force) {
    static uchar last_shield = FALSE;
    static uchar last_shieldstr = 0;
    static int last_lean_icon = -1;
    static LGPoint last_lean_pos = {-1, -1};

    LGRect r;
    LGPoint pos;
    short a, b, c, d;
    grs_bitmap *icon;
    int inum;
    bool shield = WareActive(player_struct.hardwarez_status[CPTRIP(SHIELD_HARD_TRIPLE)]) != 0;
    // int shieldstr = SHIELD_SETTING(player_struct.hardwarez_status[CPTRIP(SHIELD_HARD_TRIPLE)]);
    int shieldstr;
    char saveMode;
    bool saveBio;

    //	if(player_struct.hardwarez[CPTRIP(SHIELD_HARD_TRIPLE)]==SHIELD_VERSIONS)
    //		shieldstr=SHIELD_VERSIONS-1;

    current_meter_region = PICK_METER_REGION(full_game_3d);
    set_base_lean_bmap(shield);
    lean_icon(&pos, &icon, &inum);

    if (shield_bmap_res > 0)
        shieldstr = shield_bmap_res - RES_leanShield1;
    else
        shieldstr = 0;

    // If this is not a force update, and we're drawing the same lean icon in the same position as last
    // time, then do nothing.
    if (!force && PointsEqual(pos, last_lean_pos) && MKREF(lean_bmap_res, inum) == last_lean_icon
        && shield == last_shield && shieldstr == last_shieldstr)
        return;

    STORE_CLIP(a, b, c, d);
    ss_safe_set_cliprect(LEANOMETER_X(), LEANOMETER_Y(), LEANOMETER_X() + LEANOMETER_W, LEANOMETER_Y() + LEANOMETER_H);

    saveBio = gBioInited; // Turn off biometer while updating the lean meter.
    gBioInited = FALSE;

    if (force || last_lean_icon != -1) {
        RECT_FILL(&r, LEANOMETER_X(), LEANOMETER_Y(), LEANOMETER_X() + 46, LEANOMETER_Y() + 53);
        undraw_meter_area(&r);
    }
    RECT_FILL(&r, pos.x, pos.y, pos.x + icon->w, pos.y + icon->h);

    // saveMode = convert_use_mode;
    // convert_use_mode = 0;

    if (is_onscreen())
        uiHideMouse(&r);
    ss_bitmap(icon, r.ul.x, r.ul.y);

    if (shield) {
        grs_bitmap *sbm;

        // Get a pointer to the corresponding lean bitmap.
	FrameDesc *f = RefGet(MKREF(shield_bmap_res, inum));
	if (f != NULL) {
            f->bm.bits = (uint8_t *)(f + 1);
            sbm = &(f->bm);
        } else
            DEBUG("%s: No shield resource bitmap!", __FUNCTION__);

        // Place shield image with offset
        ss_bitmap(sbm, r.ul.x - shield_offsets[inum].x, r.ul.y - shield_offsets[inum].y);
    }

    gBioInited = saveBio;

    if (is_onscreen())
        uiShowMouse(&r);
    // convert_use_mode = saveMode;

    last_lean_pos = pos;
    last_lean_icon = MKREF(lean_bmap_res, inum);
    last_shield = shield;
    last_shieldstr = shieldstr;
    RESTORE_CLIP(a, b, c, d);
}

void draw_eye_bitmap(grs_bitmap *eye_bmap, LGPoint pos, int lasty) {
    LGRect r;
    char saveMode;

    current_meter_region = PICK_METER_REGION(full_game_3d);
    pos.x += EYEMETER_X();
    pos.y += EYEMETER_Y();
    r.ul = pos;
    r.lr.x = pos.x + eye_bmap->w;
    r.ul.y = lasty;
    r.lr.y = lasty + eye_bmap->h + 1;
    undraw_meter_area(&r);
    r.ul.y = pos.y;
    r.lr.y = pos.y + eye_bmap->h;

    // saveMode = convert_use_mode;
    // convert_use_mode = 0;
    if (is_onscreen())
        uiHideMouse(&r);
    ss_bitmap(eye_bmap, r.ul.x, r.ul.y);
    if (is_onscreen())
        uiShowMouse(&r);
    // convert_use_mode = saveMode;
}

#define HIRES_EYEMETER_H 53

void update_eye_meter(uchar force) {
    static short last_y = 0;
    static short last_ly = 0;
    static uchar last_mode = FALSE;

    short a, b, c, d;
    fix pos = eye_mods[1];
    int yang = pos % (2 * FIXANG_PI);
    short y;
    short lefty;
    grs_bitmap *eye_rbmap = eye_bmap_r(!eye_fine_mode);
    grs_bitmap *eye_lbmap = eye_bmap_l(eye_fine_mode);
    bool saveBio;

    if (yang > FIXANG_PI)
        yang -= 2 * FIXANG_PI;
    y = -(EYEMETER_H * yang / (2 * MAX_EYE_ANGLE));

    // Hey, let's take gruesome advantange of the fact that
    // booleans are zero or one.
    lefty = discrete_eye_height[1 + (yang < 0) - (yang > 0)];
    lefty -= (eye_lbmap->h) / 2;
    y += EYEMETER_H / 2 - 2 - (eye_rbmap->h / 2) / 2;

    if (!force && y == last_y && lefty == last_ly && eye_fine_mode == last_mode)
        return;
    STORE_CLIP(a, b, c, d);
    ss_safe_set_cliprect(EYEMETER_X(), EYEMETER_Y(), EYEMETER_X() + EYEMETER_W, EYEMETER_Y() + EYEMETER_H);

    saveBio = gBioInited; // Turn off biometer while updating the eye meter.
    gBioInited = FALSE;

    if (force) {
        LGRect r = {{0, 0}, {46, 53}};
        RECT_MOVE(&r, MakePoint(EYEMETER_X(), EYEMETER_Y()));
        undraw_meter_area(&r);
    }
    draw_eye_bitmap(eye_lbmap, MakePoint(2, lefty), last_ly);
    draw_eye_bitmap(eye_rbmap, MakePoint(19 - eye_rbmap->w, y), last_y);

    gBioInited = saveBio;

    RESTORE_CLIP(a, b, c, d);
    last_y = y;
    last_ly = lefty;
    last_mode = eye_fine_mode;
}

void update_meters(uchar force) {
    current_meter_region = PICK_METER_REGION(full_game_3d);
    update_eye_meter(force);
    update_lean_meter(force);
}

void zoom_to_lean_meter(void) {
    extern bool DoubleSize;

    LGPoint pos;
    LGRect start = {{-5, -5}, {+5, +5}};
    LGRect end = {{0, 0}, {LEANOMETER_W, LEANOMETER_H}};

    current_meter_region = PICK_METER_REGION(full_game_3d);
    RECT_MOVE(&end, MakePoint(LEANOMETER_X(), LEANOMETER_Y()));
    mouse_get_xy(&pos.x, &pos.y);
    if (!DoubleSize)
        ss_point_convert(&(pos.x), &(pos.y), TRUE);
    RECT_MOVE(&start, pos);
    zoom_rect(&start, &end);
}
