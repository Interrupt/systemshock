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
 * $Source: r:/prj/cit/src/RCS/gameloop.c $
 * $Revision: 1.32 $
 * $Author: dc $
 * $Date: 1994/11/19 20:35:51 $
 */

#include <string.h>

#include "mainloop.h"
#include "wares.h"
#include "hud.h"
#include "ai.h"
#include "physics.h"
#include "gamesys.h"
#include "gametime.h"
#include "status.h"
#include "render.h"
#include "musicai.h"
#include "MacTune.h"
#include "newmfd.h"
#include "faketime.h"
#include "invent.h"
#include "grenades.h"
#include "damage.h"
#include "effect.h"
#include "fullscrn.h"
#include "tools.h"
#include "olhext.h"

#include "gamescr.h"
#include "gamestrn.h"
#include "cybstrng.h"
#include "colors.h"
#include "gr2ss.h"
#include "game_screen.h"

#undef RECT_FILL
#define RECT_FILL(pr, x1, y1, x2, y2) \
    {                                 \
        (pr)->ul.x = (x1);            \
        (pr)->ul.y = (y1);            \
        (pr)->lr.x = (x2);            \
        (pr)->lr.y = (y2);            \
    }

// ----------
// GLOBALS
// ----------
uchar redraw_paused = TRUE;

// ----------
// PROTOTYPES
// ----------
void draw_pause_string(void);

long pal_frame = 0;

//------------------------------------------------------------------
void draw_pause_string(void) {
    LGRect r;
    short w, h, nw, nh;
    extern void ss_scale_string(char *s, short x, short y);

    uchar old_over = gr2ss_override;
    gr_set_fcolor(RED_BASE + 4);
    gr_set_font((grs_font *)ResGet(RES_citadelFont));
    gr_string_size(get_string(REF_STR_Pause, NULL, 0), &w, &h);
    nw = SCREEN_VIEW_X + (SCREEN_VIEW_WIDTH - w) / 2;
    nh = SCREEN_VIEW_Y + (SCREEN_VIEW_HEIGHT - h) / 2;
    RECT_FILL(&r, nw, nh, nw + w, nh + h);
    gr2ss_override = OVERRIDE_ALL;
    uiHideMouse(&r);
    ss_string(get_string(REF_STR_Pause, NULL, 0), nw, nh);
    uiShowMouse(&r);
    old_over = gr2ss_override;
}

//------------------------------------------------------------------
void game_loop(void) {
    extern void update_meters(bool);
    extern errtype check_cspace_death();
    extern uchar game_paused;
    extern void sound_frame_update(void);
    // temp
    // extern char saveArray[16];
    // if (memcmp(0, saveArray, 16))
    //     Debugger();

    // Handle paused game state
    if (game_paused) {
        if (redraw_paused) {
            Spew("gameloop", "Drawing pause!\n");
            draw_pause_string();
            redraw_paused = FALSE;
        }
        // KLC - does nothing!  loopLine(GL|0x1D,synchronous_update());
        /*if (music_on)
            loopLine(GL|0x1C, mlimbs_do_ai());
        if (pal_fx_on)
            loopLine(GL|0x1E, palette_advance_all_fx(* (long *) 0x16a)); // TickCount()*/
    }

    // If we're not paused...

    else {
        loopLine(GL | 0x10, update_state(time_passes)); // move game time

        if (time_passes) {
            Spew("gameloop", "ai_run\n");
            loopLine(GL | 0x12, ai_run());

            Spew("gameloop", "gamesys_run\n");
            loopLine(GL | 0x13, gamesys_run());

            Spew("gameloop", "advance_animations\n");
            loopLine(GL | 0x14, advance_animations());
        }
        Spew("gameloop", "wares_update\n");
        loopLine(GL | 0x16, wares_update());

        Spew("gameloop", "message_clear_check\n");
        loopLine(GL | 0x1D, message_clear_check()); // This could be done more cleverly with change flags...

        if (localChanges) {
            Spew("gameloop", "render_run\n");
            loopLine(GL | 0x1A, render_run());

            Spew("gameloop", "status_vitals_update\n");
            loopLine(GL | 0x17, if (!full_game_3d) status_vitals_update(FALSE));
            /*KLC - no longer needed
            if (_change_flag&ANIM_UPDATE)
            {
                    loopLine(GL|0x19, AnimRecur());
                    chg_unset_flg(ANIM_UPDATE);
            }
            */

            if (full_game_3d && ((_change_flag & INVENTORY_UPDATE) || (_change_flag & MFD_UPDATE)))
                _change_flag |= DEMOVIEW_UPDATE;
            if (_change_flag & INVENTORY_UPDATE) {
                Spew("gameloop", "INVENTORY_UPDATE\n");
                chg_unset_flg(INVENTORY_UPDATE);
                loopLine(GL | 0x1B, inventory_draw());
            }
            if (_change_flag & MFD_UPDATE) {
                Spew("gameloop", "MFD_UPDATE\n");
                chg_unset_flg(MFD_UPDATE);
                loopLine(GL | 0x18, mfd_update());
            }

            if (_change_flag & DEMOVIEW_UPDATE) {
                // KLC - does nothing!
                // if (sfx_on || music_on)
                //     loopLine(GL|0x1D, synchronous_update());
                chg_unset_flg(DEMOVIEW_UPDATE);
            }
        }
        if (!full_game_3d) {
            Spew("gameloop", "update_meters\n");
            loopLine(GL | 0x19, update_meters(FALSE));
        }
        if (!full_game_3d && olh_overlay_on) {
            Spew("gameloop", "olh_overlay\n");
            olh_overlay();
        }

        Spew("gameloop", "physics_run\n");
        loopLine(GL | 0x15, physics_run());
        {
            if (!olh_overlay_on && olh_active && !global_fullmap->cyber) {
                Spew("gameloop", "olh_scan_objects\n");
                olh_scan_objects();
            }
        }
        // KLC - does nothing!         loopLine(GL|0x1D,synchronous_update());
        if (sfx_on || music_on) {
            Spew("gameloop", "sound_frame_update\n");
            loopLine(GL | 0x1C, mlimbs_do_ai());
            loopLine(GL | 0x1E, sound_frame_update());
        }

        if (pal_fx_on) {
            loopLine(GL | 0x1F, palette_advance_all_fx(*tmd_ticks));
        }

        Spew("gameloop", "destroy_destroyed_objects\n");
        loopLine(GL | 0x20, destroy_destroyed_objects());
        loopLine(GL | 0x21, check_cspace_death());
    }
}
