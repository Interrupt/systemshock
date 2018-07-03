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
 * $Source: r:/prj/cit/src/RCS/vitals.c $
 * $Revision: 1.10 $
 * $Author: xemu $
 * $Date: 1994/11/04 13:10:16 $
 *
 */

#include <math.h>

#include "player.h"
#include "status.h"
#include "tools.h"
#include "colors.h"
#include "newmfd.h"
#include "gamescr.h"
#include "citres.h"
#include "gamesys.h"
#include "textmaps.h"
#include "fullscrn.h"

#include "otrip.h"

#include "gamesys.h"
#include "gr2ss.h"

// Defines
#define NUM_BIO_TRACKS 8

#define GAMESCR_BIO        0
#define GAMESCR_BIO_X      5
#define GAMESCR_BIO_Y      1
//#define GAMESCR_BIO_WIDTH  149
#define GAMESCR_BIO_WIDTH  131
#define GAMESCR_BIO_HEIGHT 17

#define DIFF_BIO        1
#define DIFF_BIO_X      6
#define DIFF_BIO_Y      181
#define DIFF_BIO_WIDTH  307
#define DIFF_BIO_HEIGHT 17

#define STATUS_CHI_AMP 8

#define STATUS_BIO_X      curr_bio_x
#define STATUS_BIO_Y      curr_bio_y
#define STATUS_BIO_WIDTH  curr_bio_w
#define STATUS_BIO_HEIGHT curr_bio_h

#define STATUS_START_OFFSET 0
#define STATUS_BIO_Y_DELTA  1
#define MAX_BIO_LENGTH      307
#define STATUS_BIO_LENGTH   (STATUS_BIO_WIDTH - STATUS_START_OFFSET)
#define STATUS_BIO_TAIL     30
#define STATUS_BIO_PEAK     (STATUS_BIO_HEIGHT - 3) // 3 because of zany art size
#define STATUS_BIO_X_BASE   (STATUS_BIO_X + STATUS_START_OFFSET)
#define STATUS_BIO_Y_BASE   (STATUS_BIO_Y + STATUS_BIO_HEIGHT - STATUS_BIO_Y_DELTA - 2)

#define SPIKE_THRESHOLD 4
#define COLOR_CHANGES   6
#define COLOR_LENGTH    (STATUS_BIO_TAIL / COLOR_CHANGES)

#define MAX_TAIL_LENGTH 5
#define NO_HEIGHT       0x1f
#define INVALID_HEIGHT  0xE0

#define COLOR_BIO_MASK  0xE0 // Top Three bits signify depth of color
#define HEIGHT_BIO_MASK 0x1f // Bottom Five bits signify height
#define BIT6            0x20 // First bit of the color field.

#define COLOR_BIT_SHIFT(x) ((x) << 5)

//#define FIND_OVERLAP(x,y) (((x) - y + STATUS_BIO_LENGTH) % STATUS_BIO_LENGTH)

#define STATUS_VITALS_X      184
#define STATUS_VITALS_Y      0
#define STATUS_VITALS_WIDTH  130
#define STATUS_VITALS_HEIGHT 17

#define STATUS_VITALS_X_BASE   (STATUS_VITALS_X + 4)
#define STATUS_VITALS_Y_TOP    (STATUS_VITALS_Y + 1)
#define STATUS_VITALS_Y_BOTTOM (STATUS_VITALS_Y + 11)
#define STATUS_VITALS_H        8
#define STATUS_VITALS_W        (STATUS_VITALS_WIDTH - 9)

#define STATUS_X      4
#define STATUS_Y      1
#define STATUS_HEIGHT 20
#define STATUS_WIDTH  312

#define GAMESCR_BIO_REF      REF_IMG_bmBiorhythm
#define DIFF_BIO_REF         REF_IMG_bmDiffBio
#define STATUS_RESID         curr_bio_ref
#define STATUS_RES_VITALSID  REF_IMG_bmVitals
#define STATUS_RES_HEALTH_ID REF_IMG_bmVitalInnardsTop
#define STATUS_RES_ENERGY_ID REF_IMG_bmVitalInnardsBottom

// Special Status Biorhythm variables

#define NO_SPIKE    0x01
#define SPIKE_NOISE 0x02

void status_vitals_start();
void status_vitals_end();
errtype draw_status_arrow(int x_coord, int y);
void draw_status_bar(ushort x0, ushort x1, ushort cutoff, ushort y);

// ===========================================================================
// ======================= * UPPER RIGHT HAND CORNER STUFF * =================
// ======================= *        STARTS HERE            * =================
// ===========================================================================

// ---------------------------------------------------------------------------
// status_vitals_init()
//
// Initially draws the background art for the vitals display, and loads in
// appropriate bitmaps so we don't hit disk randomly.

#define NUM_STATUS_ARROWS 4
#define STATUS_ANGLE_SIZE 5
grs_bitmap status_arrows[NUM_STATUS_ARROWS];

void status_vitals_init() {
    // Draw the background map
    //   draw_res_bm(STATUS_RES_VITALSID, STATUS_VITALS_X, STATUS_VITALS_Y);

    // Draw the innards
    draw_res_bm(STATUS_RES_HEALTH_ID, STATUS_VITALS_X_BASE, STATUS_VITALS_Y_TOP);
    draw_res_bm(STATUS_RES_ENERGY_ID, STATUS_VITALS_X_BASE, STATUS_VITALS_Y_BOTTOM);
    // draw_hires_resource_bm(STATUS_RES_HEALTH_ID, 372, 3);
    // draw_hires_resource_bm(STATUS_RES_ENERGY_ID, 372, 27);
    return;
}

void status_vitals_start() {
    // load in our bitmaps!
    for (int i = 0; i < NUM_STATUS_ARROWS; i++)
        simple_load_res_bitmap(&status_arrows[i], REF_IMG_bmStatusAngle1 + i);
}

void status_vitals_end() {
    int i;
    for (i = 0; i < NUM_STATUS_ARROWS; i++)
        free(status_arrows[i].bits);
}
#define VITALS_MAX 23

    // ---------------------------------------------------------------------------
    // status_vitals_update()
    //
    // This routine is called whenever the energy shield and health bar graphs
    // in the upper right hand corner of the screen need to be changed.
    //

#define STATUS_ICON_X 307

errtype status_vitals_update(uchar Full_Redraw) {
    static short last_health_x = 0;
    static short last_energy_x = 0;
    grs_bitmap *icon_bmp;
    extern uchar full_game_3d;
    Ref ref;

    short health_value, energy_value, health_x, energy_x;
    ushort minx, maxx;
    //   static long last_time=0L;
    //   long delta;

    if (global_fullmap->cyber)
        health_value = player_struct.cspace_hp;
    else
        health_value = player_struct.hit_points;

    if (health_value < 0)
        health_value = 0;

    energy_value = player_struct.energy;

    // So the scale is 0-VITALS_MAX, which is # of angles to draw
    health_x = lg_max(0, ((health_value)*VITALS_MAX + PLAYER_MAX_HP - 1) / PLAYER_MAX_HP);
    energy_x = lg_max(0, (energy_value * VITALS_MAX + MAX_ENERGY - 1) / MAX_ENERGY);
    //   mprintf("health_x = %d, energy_x = %d\n",health_x,energy_x);

    if (Full_Redraw) {
        if (health_x != 0)
            last_health_x = 0;
        if (energy_x != 0)
            last_energy_x = 0;
    }

    if (health_x != last_health_x) {
        minx = lg_min(health_x, last_health_x);
        if (Full_Redraw)
            maxx = VITALS_MAX;
        else
            maxx = lg_max(health_x, last_health_x);

        draw_status_bar(minx, maxx, health_x, STATUS_VITALS_Y_TOP);
        ref = ((global_fullmap->cyber) ? REF_IMG_bmCyberIcon1 : REF_IMG_bmHealthIcon1) + (health_x / 8);
        icon_bmp = lock_bitmap_from_ref(ref);
        ss_bitmap(icon_bmp, STATUS_ICON_X, STATUS_VITALS_Y_TOP);
        // gr_bitmap(icon_bmp, SCONV_X(STATUS_ICON_X), SCONV_Y(STATUS_VITALS_Y_TOP));
        RefUnlock(ref);

        last_health_x = health_x;
    }

    if (!(full_game_3d && global_fullmap->cyber)) {
        if (energy_x != last_energy_x) {

            minx = lg_min(energy_x, last_energy_x);
            if (Full_Redraw)
                maxx = VITALS_MAX;
            else
                maxx = lg_max(energy_x, last_energy_x);

            draw_status_bar(minx, maxx, energy_x, STATUS_VITALS_Y_BOTTOM + 1);
            ref = REF_IMG_bmEnergyIcon1 + (energy_x / 8);
            icon_bmp = lock_bitmap_from_ref(ref);
            ss_bitmap(icon_bmp, STATUS_ICON_X, STATUS_VITALS_Y_BOTTOM);
            // gr_bitmap(icon_bmp, SCONV_X(STATUS_ICON_X), SCONV_Y(STATUS_VITALS_Y_BOTTOM));
            RefUnlock(ref);

            last_energy_x = energy_x;
        }
    }

    return (OK);
}

// ---------------------------------------------------------------------------
// draw_status_arrow(int x_coord, int y)
//
// Draws a status arrow at the appropriate location, using the
// right bitmap for that location.  A negative x_coord means to "undraw"
// an angle at that coordinate.
// NOTE:  x_coord in in angle units, not pixels!  y is still in pixels
errtype draw_status_arrow(int x_coord, int y) {
    int index;
    if (x_coord < 0) {
        index = 3;
        x_coord = ~x_coord;
    } else if (x_coord <= 7)
        index = 0;
    else if (x_coord <= 15)
        index = 1;
    else
        index = 2;
    ss_bitmap(&status_arrows[index], STATUS_VITALS_X_BASE + (x_coord * STATUS_ANGLE_SIZE), y);
    // gr_bitmap(&status_arrows[index],
    //					SCONV_X(STATUS_VITALS_X_BASE + (x_coord * STATUS_ANGLE_SIZE)),
    //					SCONV_Y(y));
    return (OK);
}

// ---------------------------------------------------------------------------
// draw_status_bar()
//
// Draws a series of status angles at the specified height, within the specified coordinate
// range, color shaded as appropriate.

void draw_status_bar(ushort x0, ushort x1, ushort cutoff, ushort y) {
    int i;
    LGRect r;

    r.ul = MakePoint(STATUS_VITALS_X_BASE + (x0 * STATUS_ANGLE_SIZE), y);
    r.lr = MakePoint(STATUS_VITALS_X_BASE + (x1 * STATUS_ANGLE_SIZE), y + status_arrows[0].h);

    uiHideMouse(&r);
    //   mprintf ("draw_bar x0=%d x1=%d cutoff = %d\n",x0,x1,cutoff);
    // Do the drawing
    for (i = x0; i < cutoff; i++)
        draw_status_arrow(i, y);

    // Do the erasing
    if (!full_game_3d)
        for (i = cutoff; i < x1; i++)
            draw_status_arrow(~i, y);
    uiShowMouse(&r);

    return;
}
