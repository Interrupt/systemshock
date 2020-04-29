
#include "mouselook.h"
#include "mouse.h"
#include "player.h"
#include "physics.h"
#include "froslew.h"
#include "objsim.h"
#include "Prefs.h"

#include <stdio.h>
#include <SDL.h>

float mlook_hsens = 250;
float mlook_vsens = 50;

int mlook_vel_x, mlook_vel_y;

extern void pump_events();
extern void player_set_eye_fixang(int ang);
extern void physics_set_relax(int axis, uchar relax);
extern void player_set_eye(byte);
extern byte player_get_eye();

extern uchar game_paused;
extern short mouseInstantX, mouseInstantY;
extern int32_t eye_mods[3];

extern SDL_Window *window;
extern SDL_Renderer *renderer;

void middleize_mouse(void);
void get_mouselook_vel(int *vx, int *vy);

int mlook_enabled = FALSE;

void mouse_look_physics() {

	if (game_paused || !global_fullmap || !mlook_enabled) return;

	middleize_mouse();

    int mvelx, mvely;
	get_mouselook_vel(&mvelx, &mvely);

    if (global_fullmap->cyber) {
        //see physics_run() in physics.c
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

bool TriggerRelMouseMode = FALSE;

void mouse_look_toggle(void)
{
    mlook_enabled = !mlook_enabled;

	if (mlook_enabled)
	{
		SDL_SetRelativeMouseMode(SDL_TRUE);

		//throw away this first relative mouse reading
		int mvelx, mvely;
		get_mouselook_vel(&mvelx, &mvely);
	}
	else
	{
		SDL_SetRelativeMouseMode(SDL_FALSE);

		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		SDL_WarpMouseInWindow(window, w/2, h/2);

		TriggerRelMouseMode = TRUE;
	}
}

void mouse_look_off(void)
{
	if (mlook_enabled)
	{
		mlook_enabled = FALSE;

		SDL_SetRelativeMouseMode(SDL_FALSE);

		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		SDL_WarpMouseInWindow(window, w/2, h/2);

		TriggerRelMouseMode = TRUE;
	}
}

void mouse_look_unpause(void)
{
	if (mlook_enabled)
	{
		SDL_SetRelativeMouseMode(SDL_TRUE);

		//throw away this first relative mouse reading
		int mvelx, mvely;
		get_mouselook_vel(&mvelx, &mvely);
	}
}
