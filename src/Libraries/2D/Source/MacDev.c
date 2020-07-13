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
// device driver for standard Mac 640x480 256 color mode
#include "grnull.h"
#include "grd.h"
#include "lg.h"
#include "MacDev.h"
#include "cnvdrv.h"
#include "cnvtab.h"
#include "fcntab.h"
#include "lintab.h"

// NOTE!
//
// MacDev (and some other 2d code) relies on the fact that several globals are setup
// in the main game code (InitMac.c and ShockBitmap.c).  gScreenAddress & gScreenRowbytes
// are assumed to point to the right part of the main screen, and gMainColorHand is
// assumed to be the main color table handle for the game.  So make sure you call CheckConfig
// and SetupOffscreenBitmaps before you start up the 2D system.  This stuff should be inside
// the 2D system itself, but its kind of outside of the realm of the 2D library as it exists,
// so its not here.
//

typedef void (**ptr_type)();

// Mac device function table
void (**mac_device_table[])() = {
    (ptr_type)gr_null,       // init device
    (ptr_type)gr_null,       // close device
    (ptr_type)mac_set_mode,  // set mode
    (ptr_type)gr_null,       // get mode
    (ptr_type)mac_set_state, // set state
    (ptr_type)mac_get_state, // get state
    (ptr_type)gr_null,       // was mac_stat_htrace
    (ptr_type)gr_null,       // was mac_stat_vtrace
    (ptr_type)mac_set_pal,   // set palette
    (ptr_type)mac_get_pal,   // get palette
    (ptr_type)gr_null,       // set width
    (ptr_type)gr_null,       // get width
    (ptr_type)gr_null,       // set focus, was mac_set_focus
    (ptr_type)gr_null,       // get focus, was mac_get_focus
    (ptr_type)gr_null,       // canvas table
    (ptr_type)gr_null        // span table
};

//========================================================================
// Mac specific device routines
//========================================================================

extern intptr_t *gScreenAddress;

//------------------------------------------------------------------------
// init the graphics mode, set up function tables and screen base address
//
void mac_set_mode(void) {
    DEBUG("%s: Initializing graphics mode", __FUNCTION__ );
    grd_mode_cap.vbase = gScreenAddress;

    grd_canvas_table_list[BMT_DEVICE] = flat8_canvas_table;
    grd_function_table_list[BMT_DEVICE] = (grt_function_table *)flat8_function_table;
    grd_uline_fill_table_list[BMT_DEVICE] = (grt_uline_fill_table *)flat8_uline_fill_table;
}

//------------------------------------------------------------------------
// set the color palette (copy entries into gMainColorHand, then call SetEntries
// and ResetCTSeed).

uchar backup_pal[768];
void mac_set_pal(int start, int n, uint8_t *pal_data) {
    extern void SetSDLPalette(int index, int count, uchar *pal);

    // HAX: Only update when given a whole palette!
    if (start == 0 && n == 256) {
        SetSDLPalette(start, n, pal_data);

        // Save the palette
        memmove(&backup_pal, pal_data, sizeof(uchar) * 768);
    }
}

//------------------------------------------------------------------------
// get the current color palette (copy entries from gMainColorHand)
void mac_get_pal(int start, int n, uint8_t *pal_data) {
    memmove(pal_data, &backup_pal, sizeof(uchar) * 768);
}

// set and get state don't currently do anything, since its not apparent
// any of the stuff from the VGA driver (text mode, palette stuff) is necessary
// (if the game uses these calls to remember palettes between changes we may need
// to implement something here
//------------------------------------------------------------------------
int mac_set_state(void *buf, int clear) { return (0); }

//------------------------------------------------------------------------
int mac_get_state(void *buf, int flags) { return (0); }
