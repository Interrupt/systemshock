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
#ifndef __TOOLS_H
#define __TOOLS_H

#include "lg_types.h"
#include "error.h"
#include "res.h"
#include "fix.h"
#include "region.h"
#include "2d.h"

/*
 * $Source: r:/prj/cit/src/inc/RCS/tools.h $
 * $Revision: 1.27 $
 * $Author: xemu $
 * $Date: 1994/10/30 06:07:38 $
 *
 */

// Includes

// C Library Includes

// System Library Includes

// Defines

#define STORE_CLIP(a, b, c, d) \
    (a) = gr_get_clip_l();     \
    (b) = gr_get_clip_t();     \
    (c) = gr_get_clip_r();     \
    (d) = gr_get_clip_b()

#define RESTORE_CLIP(a, b, c, d) gr_set_cliprect((a), (b), (c), (d))

// Prototypes

// Draw a resouce bitmap at the x,y coordinates, without doing any pallet or
// mouse tricks.
errtype draw_raw_resource_bm(Ref id, int x, int y);

// same thing, down to error handling. FIXME do we need both of these? (Or
// indeed either.)
errtype draw_raw_res_bm_temp(Ref id, int x, int y);

// Draw a resource bitmap at the x,y coordinates, loading the pallet (if available)
// and doing appropriate mouse tricks.
void draw_hires_resource_bm(Ref id, int x, int y);
void draw_hires_halfsize_bm(Ref id, int x, int y);
errtype draw_res_bm(Ref id, int x, int y);
errtype draw_res_bm_core(Ref id, int x, int y, uchar scale);
errtype draw_full_res_bm(Ref id, int x, int y, uchar fade_in);

// Return the width or height of a resource bitmap.
int res_bm_width(Ref id);
int res_bm_height(Ref id);

// Draw a Text string to the screen, given a resource font pointer
#define res_draw_text(font, text, x, y) res_draw_text_shadowed(font, text, x, y, FALSE)
errtype res_draw_text_shadowed(Id id, char *text, int x, int y, uchar shadow);

// Like res_draw_text, but takes a string number instead.
errtype res_draw_string(Id font, int strid, int x, int y);

// hmmm, why dont these work, eh
// note the void's so we dont need LGRect.h in here, neat huh?
void Rect_gr_box(LGRect *rv);
void Rect_gr_rect(LGRect *rv);

// Dump the current screen out to a .GIF in the GEN directory
uchar gifdump_func(short keycode, ulong context, void *data);

// Spit up a box containing a message.
errtype message_box(char *box_text);

// Writes a message to the info LGRegion
errtype string_message_info(int strnum);
errtype message_info(const char *info_text);
errtype message_clear_check();

// Spit up a box asking for confirmation.  Returns true or false, accordingly.
uchar confirm_box(char *box_text);

// From the short-lived util.c
// ¥¥¥FILE *fopen_gen(char *fname, const char *how);
int open_gen(char *fname, int access1, int access2);
char *next_number_fname(char *fname);
//¥¥¥Êchar *next_number_dpath_fname(Datapath *dpath, char *fname);

// Execute a tight loop, doing appropriate music/palette things
errtype tight_loop(uchar check_input);

// set / unset "wait" cursor
errtype begin_wait();
errtype end_wait();

// search/replace characters in string
void string_replace_char(char *s, char from, char to);

fixang point_in_view_arc(fix target_x, fix target_y, fix looker_x, fix looker_y, fixang look_facing, fixang *real_dir);

// our very own strtoupper!
void strtoupper(char *text);

// KLC - moved here from WRAPPER.H.
void gamma_dealfunc(ushort gamma_qvar);

// KLC - added
void second_format(int sec_remain, char *s);

int hyphenated_wrap_text(char *ps, char *out, short width);

int str_to_hex(char val);
void strip_newlines(char *buf);

void text_button(char *text, int xc, int yc, int col, int shad, int w, int h);

void zoom_rect(LGRect *start, LGRect *end);

void ZoomDrawProc(int erase);

// Globals

#endif // __TOOLS_H
