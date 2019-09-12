/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.
Copyright (C) 2019 Shockolate Project

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
 * $Source: r:/prj/lib/src/input/RCS/kbcook.c $
 * $Revision: 1.5 $
 * $Author: kaboom $
 * $Date: 1994/08/15 16:35:59 $
 *
 * Routines to convert raw scan codes to more useful cooked codes.
 *
 * This file is part of the input library.
 */
#include "lg.h"
#include "kbcook.h"
#include "keydefs.h"
//#include <kbmod.h>
//#include <kbscan.h>

//----------------------------------------------------------------------------
// This cooks kbc codes into ui codes which include ascii stuff.
//----------------------------------------------------------------------------
//  For Mac version, replace the whole thing, because the "cooked" info
//  already exists in the kbs_event record.  So just format the results
//  as expected.
//----------------------------------------------------------------------------
errtype kb_cook(kbs_event ev, ushort *cooked, uchar *results) {
    // On the Mac, since modifiers by themselves don't produce an event,
    // you always have a "cooked" result.

    *results = TRUE;
    *cooked = ev.ascii;

    if (ev.ascii == 0) {
        // FIXME: Why don't the arrow keys translate properly?
        if (ev.code == 125)
            *cooked = KEY_DOWN;
        else if (ev.code == 126)
            *cooked = KEY_UP;
        else if (ev.code == 123)
            *cooked = KEY_LEFT;
        else if (ev.code == 124)
            *cooked = KEY_RIGHT;
        else if (ev.code == 13)
            *cooked = KEY_ENTER;
    }

    *cooked |= (short)ev.state << KB_DOWN_SHF; // Add in the key-down state.

    // text input events are used to get printable characters (see sdl_events.c)
    // note that text input events don't work for ctrl'd or alt'd keys

    if (ev.modifiers & KB_MOD_CTRL) // If command-key was down,
        *cooked |= KB_FLAG_CTRL;    // simulate a control key

    if (ev.modifiers & KB_MOD_SHIFT) // If shift-key was down
        *cooked |= KB_FLAG_SHIFT;

    if (ev.modifiers & KB_MOD_ALT) // If option-key was down,
        *cooked |= KB_FLAG_ALT;    // simulate an alt key.

    return OK;
    /*
       ushort flags = KB_CNV(ev.code,0);
       uchar shifted = 1 & ((kbd_modifier_state >> KBM_SHIFT_SHF)
          | (kbd_modifier_state >> (KBM_SHIFT_SHF+1)));
       uchar capslock = 1 & (flags >> CNV_CAPS_SHF)
                         & (kbd_modifier_state >> KBM_CAPS_SHF);
       ushort cnv = KB_CNV(ev.code,shifted ^ capslock);
       int old_mods = kbd_modifier_state;

       *cooked = cnv & (CNV_SPECIAL|CNV_2ND|0xFF)  ;
       *results = FALSE;

       // if an up event, use negative logic.  Wacky
       if (ev.state == KBS_UP) kbd_modifier_state = ~kbd_modifier_state;
       switch(ev.code)  // check for modifiers
       {
       case 0x7a: return 0; break;
       case KBC_LSHIFT:
          kbd_modifier_state |= KBM_LSHIFT;
          break;
       case KBC_RSHIFT:
          kbd_modifier_state |= KBM_RSHIFT;
          break;
       case KBC_LCTRL:
          kbd_modifier_state |= KBM_LCTRL;
          break;
       case KBC_RCTRL:
          kbd_modifier_state |= KBM_RCTRL;
          break;
       case KBC_CAPS:
          if (ev.state == KBS_DOWN)
             kbd_modifier_state ^= KBM_CAPS;
          break;
       case KBC_NUM:
          if (ev.state == KBS_DOWN)
            kbd_modifier_state ^= KBM_NUM;
          break;
       case KBC_SCROLL:
          if (ev.state == KBS_DOWN)
             kbd_modifier_state ^= KBM_SCROLL;
          break;
       case KBC_LALT:
          kbd_modifier_state |= KBM_LALT;
          break;
       case KBC_RALT:
          kbd_modifier_state |= KBM_RALT;
          break;
       default:
          *results = TRUE;  // Not a modifier key, we must translate.
          break;
       }
       if (ev.state == KBS_UP) kbd_modifier_state = ~kbd_modifier_state;
       if ((kbd_modifier_state&KBM_LED_MASK) != (old_mods&KBM_LED_MASK))
          kb_set_leds(kbd_modifier_state&KBM_LED_MASK);
       if (!*results) return OK;

       if ((cnv & CNV_NUM) && !(kbd_modifier_state & KBM_NUM))
          *cooked = ev.code|KB_FLAG_SPECIAL;

       *cooked |= (short)ev.state << KB_DOWN_SHF;

       *cooked |= (((kbd_modifier_state << (KB_CTRL_SHF - KBM_CTRL_SHF))
          | (kbd_modifier_state << (KB_CTRL_SHF - KBM_CTRL_SHF-1)))
             & KB_FLAG_CTRL) & cnv;

       *cooked |= (((kbd_modifier_state << (KB_ALT_SHF - KBM_ALT_SHF))
          | (kbd_modifier_state << (KB_ALT_SHF - KBM_ALT_SHF-1)))
             & KB_FLAG_ALT) & cnv;

       // if KB_FLAG_SPECIAL is set, then let set the shifted
       // flag according to shifted
       *cooked |= (shifted << KB_SHIFT_SHF) & cnv;
       return OK;
    */
}

uchar kb_get_cooked(ushort *key) {
    uchar res = FALSE;
    kbs_event ev = kb_next();
    if (ev.code == KBC_NONE)
        return res;
    kb_cook(ev, key, &res);
    return res;
}
