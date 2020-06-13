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
#ifndef __WRAPPER_H
#define __WRAPPER_H

/*
 * $Source: n:/project/cit/src/inc/RCS/wrapper.h $
 * $Revision: 1.7 $
 * $Author: dc $
 * $Date: 1994/05/12 00:30:52 $
 *
 * $Log: wrapper.h $
 * Revision 1.7  1994/05/12  00:30:52  dc
 * comment defines
 *
 * Revision 1.6  1994/05/11  21:31:04  dc
 * some externals, if i remember correctly
 *
 * Revision 1.5  1994/04/07  22:37:53  xemu
 * new wrapper paradigm
 *
 * Revision 1.4  1994/02/25  15:43:58  mahk
 * Added stupid terseness questbit.
 *
 * Revision 1.3  1993/09/02  23:08:56  xemu
 * angle me baby
 *
 * Revision 1.2  1993/07/22  17:31:25  xemu
 * some #defines
 *
 * Revision 1.1  1993/06/14  15:37:04  xemu
 * Initial revision
 *
 *
 */

// Includes

// Defines
#define MAIN_PANEL 0
#define SAVELOAD_PANEL 1

// questbit for terse text.
#define TERSENESS_QBIT 0x180

// Prototypes
uchar demo_quit_func(ushort keycode, uint32_t context, intptr_t data);

errtype make_options_cursor(void);

#ifdef AUDIOLOGS
void recompute_audiolog_level(ushort vol);
#endif
void recompute_digifx_level(ushort vol);
void recompute_music_level(ushort vol);

void digichan_dealfunc(short val);

void language_change(uchar lang);

void screenmode_screen_init(void);

void wrapper_start(void (*init)(void));

// Replaces the inventory panel with a wrapper input paneloid thing,
// which is 2 by width text buttons for the user to click on.  When clicked,
// the passed callback is called with the number of the button clicked
// as an argument.
uchar wrapper_options_func(ushort keycode, uint32_t context, intptr_t data);

errtype wrapper_create_mouse_region(LGRegion *root);

#define NUM_SAVE_SLOTS 8
#define SAVE_COMMENT_LEN 32

// Globals
extern char save_game_name[];
extern char comments[NUM_SAVE_SLOTS + 1][SAVE_COMMENT_LEN];

#define Poke_SaveName(game_num)                    \
    {                                              \
        save_game_name[6] = '0' + (game_num >> 3); \
        save_game_name[7] = '0' + (game_num & 7);  \
    }

enum TEMP_STR_ {
    REF_STR_Renderer = 0x10000000,
    REF_STR_Software,
    REF_STR_OpenGL,

    REF_STR_TextFilt = 0x10000010,
    REF_STR_TFUnfil, // unfiltered
    REF_STR_TFBilin, // bilinear

    REF_STR_MousLook = 0x11000000,
    REF_STR_MousNorm,
    REF_STR_MousInv,

    REF_STR_Seqer    = 0x20000000,
    REF_STR_ADLMIDI,
    REF_STR_NativeMI,
#ifdef USE_FLUIDSYNTH
    REF_STR_FluidSyn,
#endif // USE_FLUIDSYNTH
    REF_STR_MidiOut  = 0x2fffffff,

    REF_STR_MidiOutX = 0x30000000 // 0x30000000-0x3fffffff are MIDI outputs
};

#endif // __WRAPPER_H
