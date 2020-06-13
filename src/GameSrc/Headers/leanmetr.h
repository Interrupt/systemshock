/*

Copyright (C) 2020 Shockolate Project

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

#ifndef LEANMETR_H
#define LEANMETR_H

// ---------
// PROTOTYPES
// ---------
void set_base_lean_bmap(uchar shield);
fix compute_filter_weight(ulong deltat);
fix apply_weighted_filter(fix input, fix state, ulong deltat);
void slam_posture_meter_state(void);
fix velocity_crouch_filter(fix crouch);
void lean_icon(LGPoint *pos, grs_bitmap **icon, int *inum);
void player_reset_eye(void);
byte player_get_eye(void);
void player_set_eye_fixang(int ang);
int player_get_eye_fixang(void);
uchar eye_mouse_handler(uiEvent *ev, LGRegion *r, intptr_t);
uchar lean_mouse_handler(uiEvent *ev, LGRegion *r, intptr_t);
void init_posture_meters(LGRegion *root, uchar fullscreen);
void update_lean_meter(uchar force);
void draw_eye_bitmap(grs_bitmap *eye_bmap, LGPoint pos, int lasty);
void update_eye_meter(uchar force);
void update_meters(uchar force);
void zoom_to_lean_meter(void);

#endif
