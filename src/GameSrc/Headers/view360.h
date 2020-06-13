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

#include "mfdint.h"

// render contexts for MFD and invent panels.
#define NUM_360_CONTEXTS 3

// ------------------------------------
// WHOOP WHOOP HACK HACK HACK ALERT
// left & right contexts IDs must be the same as MFD ids.

#define LEFT_CONTEXT  0
#define RIGHT_CONTEXT 1
#define MID_CONTEXT   2

// Hey, let's expose the rep of the fullscreen visible bits.
// (see fullscrn.h)
#define VISIBLE_BIT(c) (1u << (c))

#define MODE_360  0 // All 3 views
#define MODE_270  1 // Just side views
#define MODE_REAR 2 // Just rear view in mfd

#define REAR_FOV 110

extern uchar view360_active_contexts[NUM_360_CONTEXTS]; // which contexts should actually draw
extern uchar view360_context_views[NUM_360_CONTEXTS];   // which view is being shown by a given context

void view360_init(void);
void view360_shutdown(void);
void mfd_view360_expose(MFD *mfd, ubyte control);
uchar inv_is_360_view(void);
void view360_update_screen_mode(void);
void view360_render(void);
void view360_setup_mode(uchar mode);
void view360_restore_inventory(void);
int view360_fullscrn_draw_callback(void *, void *vbm, int x, int y, int flg);
void view360_turnon(uchar visible, uchar real_start);
void view360_turnoff(uchar visible, uchar real_stop);
bool view360_check(void);
