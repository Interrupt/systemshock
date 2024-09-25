/*

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

#include <SDL.h>

#include "leanmetr.h"
#include "mouselook.h"
#include "mouse.h"
#include "player.h"
#include "physics.h"
#include "objsim.h"
#include "Prefs.h"

float mlook_hsens = 250;
float mlook_vsens = 50;

int mlook_vel_x, mlook_vel_y;

extern uchar game_paused;
extern short mouseInstantX, mouseInstantY;
extern int32_t eye_mods[3];

extern SDL_Window *window;
extern SDL_Renderer *renderer;

void middleize_mouse(void);
void get_mouselook_vel(int *vx, int *vy);

int mlook_enabled = FALSE;

void mouse_look_physics() {

    if (game_paused || !global_fullmap || !mlook_enabled)
        return;

    middleize_mouse();

    int mvelx, mvely;
    get_mouselook_vel(&mvelx, &mvely);

    if (global_fullmap->cyber) {
        // see physics_run() in physics.c
        mlook_vel_x = -mvelx;
        mlook_vel_y = -mvely;
    } else {
        // player head controls
        mvelx *= -mlook_hsens;
        mvely *= (gShockPrefs.goInvertMouseY ? mlook_vsens : -mlook_vsens);

        if (mvely != 0) {
            // Moving the eye up angle is easy
            fix pos = player_struct.eye_pos + mvely;
            player_set_eye_fixang(pos);
            physics_set_relax(CONTROL_YZROT, FALSE);
        }

        if (mvelx != 0) {
            EDMS_mouselook(objs[PLAYER_OBJ].info.ph, mvelx);
        }
    }
}

bool TriggerRelMouseMode = FALSE;

void mouse_look_toggle(void) {
    mlook_enabled = !mlook_enabled;

    if (mlook_enabled) {
        SDL_SetRelativeMouseMode(SDL_TRUE);

        // throw away this first relative mouse reading
        int mvelx, mvely;
        get_mouselook_vel(&mvelx, &mvely);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_WarpMouseInWindow(window, w / 2, h / 2);

        TriggerRelMouseMode = TRUE;
    }
}

void mouse_look_off(void) {
    if (mlook_enabled) {
        mlook_enabled = FALSE;

        SDL_SetRelativeMouseMode(SDL_FALSE);

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_WarpMouseInWindow(window, w / 2, h / 2);

        TriggerRelMouseMode = TRUE;
    }
}

void mouse_look_unpause(void) {
    if (mlook_enabled) {
        SDL_SetRelativeMouseMode(SDL_TRUE);

        // throw away this first relative mouse reading
        int mvelx, mvely;
        get_mouselook_vel(&mvelx, &mvely);
    }
}
