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

#ifndef CURSORS_H
#define CURSORS_H

uchar cursor_get_callback(LGRegion* reg, LGRect* rect, void* vp);

errtype ui_init_cursor_stack(uiSlab* slab, LGCursor* default_cursor);
errtype ui_init_cursors(void);
errtype ui_shutdown_cursors(void);
uchar ui_set_current_cursor(LGPoint pos);
void ui_update_cursor(LGPoint pos);

#endif
