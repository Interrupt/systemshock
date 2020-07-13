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
 * $Source: n:/project/cit/src/inc/RCS/amaploop.h $
 * $Revision: 1.1 $
 * $Author: dc $
 * $Date: 1994/04/06 07:27:10 $
 */

#include "amap.h"

#define AMAP_FULLEXPOSE (LL_CHG_BASE << 1)
#define AMAP_MAP_EV     (LL_CHG_BASE << 2)
#define AMAP_BUTTON_EV  (LL_CHG_BASE << 3)
#define AMAP_MESSAGE_EV (LL_CHG_BASE << 4)

uchar amap_ms_callback(curAMap *amptr, int x, int y, short action, ubyte but);
uchar amap_scroll_handler(uiEvent *ev, LGRegion *r, intptr_t user_data);
void automap_loop(void);
char *fsmap_get_lev_str(char *buf, int siz);
void fsmap_startup(void);
void fsmap_free(void);
