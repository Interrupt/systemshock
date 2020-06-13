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
#ifndef __NEWMFD_H
#define __NEWMFD_H

/*
 * $Source: newmfd.h

 * $Revision: 1.3

 * $Author: spaz

 * $Date: 7/13/93

 *
 */

// This file is now a placeholder.

#include "mfdext.h"

void cap_mfds_with_func(uchar func, uchar max);
void fullscreen_refresh_mfd(ubyte mfd_id);
void mfd_change_fullscreen(uchar on);
int mfd_choose_func(int my_func, int my_slot);
errtype mfd_clear_all();
void mfd_draw_button_panel(ubyte mfd_id);
ubyte mfd_get_func(ubyte mfd_id, ubyte s);
uchar mfd_scan_opacity(int mfd_id, LGPoint epos);
errtype mfd_update_screen_mode();
void mfd_zoom_rect(LGRect *start, int mfdnum);
void mfd_language_change(void);

#endif // NEWMFD_H
