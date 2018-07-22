
#include "mouselook.h"
#include "mouse.h"
#include "player.h"
#include "physics.h"
#include "froslew.h"
#include "objsim.h"
#include <stdio.h>
#include <SDL.h>

int mlook_enabled = FALSE;
int mlook_hsens = 250;
int mlook_vsens = 50;

int mouse_look_xvel;
int mouse_look_yvel;

extern void pump_events();
extern void player_set_eye_fixang(int ang);
extern void physics_set_relax(int axis, uchar relax);
extern void player_set_eye(byte);
extern byte player_get_eye();

extern uchar game_paused;
extern short mouseInstantX, mouseInstantY;
extern long eye_mods[3];

void mouse_look_stop() {
    if (!mlook_enabled && !game_paused) {
        mlook_enabled = false;
        center_mouse();
    }
}

void mouse_look_physics() {
    mlook_vel_x = mlook_vel_y = 0;

    if (game_paused || !global_fullmap) {
        return;
    }

    if (!mlook_enabled)
        return;

    short mx, my;
    int mvelx, mvely;
    short middle_x = (grd_cap->w / 2);
    short middle_y = (grd_cap->h / 2);

    // Where is the mouse this time?
    mouse_get_xy(&mx, &my);

    // How far have we moved?
    mvelx = mlook_vel_x = (middle_x - mx);
    mvely = mlook_vel_y = (middle_y - my);

    mvelx *= mlook_hsens;
    mvely *= mlook_vsens;

    // Can put the mouse back now
    if (mvelx != 0 || mvely != 0)
        mouse_put_xy(middle_x, middle_y);

    if (global_fullmap->cyber == FALSE) {
        // player head controls

        if (mvely != 0) {
            // Moving the eye up angle is easy
            fix pos = player_struct.eye_pos + mvely;
            player_set_eye_fixang(pos);
            physics_set_relax(CONTROL_YZROT, FALSE);
        }

        if (mvelx != 0) {
            Obj *cobj = &objs[PLAYER_OBJ];

            // Turning the player is harder, need to update the physics state

            // Grab physics state
            State current_state;
            EDMS_get_state(objs[PLAYER_OBJ].info.ph, &current_state);

            // Turn us a bit
            current_state.alpha += mvelx;

            // Now put the player there
            EDMS_holistic_teleport(objs[PLAYER_OBJ].info.ph, &current_state);
        }
    }
}

void center_mouse() {
    short middle_x = (grd_cap->w / 2);
    short middle_y = (grd_cap->h / 2);
    mouse_put_xy(middle_x, middle_y);
}

void mouse_look_toggle() {
    mlook_enabled = !mlook_enabled;

    if (mlook_enabled) {
        // Flush mouse events, because we don't care about the past anymore
        pump_events();
        mouse_flush();

        // Now we can center the mouse
        center_mouse();
    }
}

void mouse_look_off() { mlook_enabled = FALSE; }

void mouse_look_unpause() {
    if (mlook_enabled)
        center_mouse();
}