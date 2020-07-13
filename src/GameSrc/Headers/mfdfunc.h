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

#ifndef MFDFUNC_H
#define MFDFUNC_H

#include "mfdint.h"

void draw_mfd_item_spew(Ref id, int n);
char *level_to_floor(int lev_num, char *buf);
int mfd_bmap_id(int triple);
uchar mfd_distance_remove(ubyte slot_func);
void mfd_item_micro_expose(uchar full, int triple);

void install_keypad_hotkeys(void);
void mfd_setup_keypad(char special);

void update_item_mfd(void);
uchar mfd_target_qual(void);
uchar mfd_automap_qual(void);
uchar mfd_weapon_qual(void);
void weapon_mfd_for_reload(void);

void mfd_setup_elevator(ushort levmask, ushort reachmask, ushort curlevel, uchar special);
void mfd_elevator_expose(MFD *mfd, ubyte control);

#endif
