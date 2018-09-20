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
 * $Source: r:/prj/cit/src/RCS/movekeys.c $
 * $Revision: 1.25 $
 * $Author: mahk $
 * $Date: 1994/11/22 22:53:07 $
 *
 */

#include <stdlib.h>

#include "input.h"
#include "player.h"
#include "physics.h"
#include "gamesys.h"
#include "weapons.h"
#include "movekeys.h"



#define KEYBD_CONTROL_BANK  1



//filled in MacSrc/Prefs.c
MOVE_KEYBIND      MoveKeybinds[MAX_MOVE_KEYBINDS+1];
MOVE_KEYBIND MoveCyberKeybinds[MAX_MOVE_KEYBINDS+1];

static uchar motion_key_scancodes[256+1];
static byte poll_controls[6];

extern Boolean gKeypadOverride;



uchar parse_motion_key(ushort code, short *cnum, short *cval);
uchar parse_motion_key_cyber(ushort code, short *cnum, short *cval);
void init_motion_polling(void);
void setup_motion_polling(void);
void process_motion_keys(void);
uchar motion_keycheck_handler(uiEvent *ev, LGRegion *, void *);

extern void physics_set_relax(int axis, uchar relax);



uchar parse_motion_key(ushort code, short *cnum, short *cval)
{
  int i = 0, move = -1;

  *cnum = -1;
  *cval = 0;

  while (MoveKeybinds[i].code != 255)
  {
    if (code == MoveKeybinds[i].code) {move = MoveKeybinds[i].move; break;}
    i++;
  }

  switch (move)
  {
    case M_RUNFORWARD:
      *cnum = CONTROL_YVEL;
      *cval = CONTROL_MAX_VAL;
    break;

    case M_FORWARD:
      *cnum = CONTROL_YVEL;
      *cval = CONTROL_MAX_VAL / 2;
    break;

    case M_FASTTURNLEFT:
      *cnum = CONTROL_XYROT;
      *cval = -CONTROL_MAX_VAL;
    break;

    case M_TURNLEFT:
      *cnum = CONTROL_XYROT;
      *cval = -CONTROL_MAX_VAL / 2;
    break;

    case M_FASTTURNRIGHT:
      *cnum = CONTROL_XYROT;
      *cval = CONTROL_MAX_VAL;
    break;

    case M_TURNRIGHT:
      *cnum = CONTROL_XYROT;
      *cval = CONTROL_MAX_VAL / 2;
    break;

    case M_BACK:
      *cnum = CONTROL_YVEL;
      *cval = -CONTROL_MAX_VAL / 2;
    break;

    case M_SLIDELEFT:
      *cnum = CONTROL_XVEL;
      *cval = -CONTROL_MAX_VAL / 2;
    break;

    case M_SLIDERIGHT:
      *cnum = CONTROL_XVEL;
      *cval = CONTROL_MAX_VAL / 2;
    break;

    case M_JUMP:
      *cnum = CONTROL_ZVEL;
      *cval = MAX_JUMP_CONTROL;
    break;

    case M_LEANUP:
      *cnum = CONTROL_XZROT;
      *cval = 0;
      physics_set_relax(*cnum, TRUE);
    break;

    case M_LEANLEFT:
      *cnum = CONTROL_XZROT;
      *cval = -CONTROL_MAX_VAL;
      physics_set_relax(*cnum, TRUE);
    break;

    case M_LEANRIGHT:
      *cnum = CONTROL_XZROT;
      *cval = CONTROL_MAX_VAL;
      physics_set_relax(*cnum, TRUE);
    break;

    case M_LOOKUP:
      *cnum = CONTROL_YZROT;
      *cval = CONTROL_MAX_VAL;
    break;

    case M_LOOKDOWN:
      *cnum = CONTROL_YZROT;
      *cval = -CONTROL_MAX_VAL;
    break;

    case M_RUNLEFT:
      *cnum = CONTROL_YVEL;
      *cval = CONTROL_MAX_VAL;
      if (abs(poll_controls[*cnum]) < abs(*cval)) poll_controls[*cnum] = *cval;
      *cnum = CONTROL_XYROT;
      *cval = -CONTROL_MAX_VAL / 2;
    break;

    case M_RUNRIGHT:
      *cnum = CONTROL_YVEL;
      *cval = CONTROL_MAX_VAL;
      if (abs(poll_controls[*cnum]) < abs(*cval)) poll_controls[*cnum] = *cval;
      *cnum = CONTROL_XYROT;
      *cval = CONTROL_MAX_VAL / 2;
    break;
  }

  return *cnum != -1;
}



uchar parse_motion_key_cyber(ushort code, short *cnum, short *cval)
{
  int i = 0, move = -1;

  code &= ~KB_FLAG_2ND;

  *cnum = -1;
  *cval = 0;

  while (MoveCyberKeybinds[i].code != 255)
  {
    if (code == MoveCyberKeybinds[i].code) {move = MoveCyberKeybinds[i].move; break;}
    i++;
  }

  switch (move)
  {
    case M_THRUST:
      *cnum = CONTROL_ZVEL;
      *cval = MAX_JUMP_CONTROL;
    break;

    case M_CLIMB:
      *cnum = CONTROL_YVEL;
      *cval = -CONTROL_MAX_VAL;
    break;

    case M_BANKLEFT:
      *cnum = CONTROL_XYROT;
      *cval = -CONTROL_MAX_VAL;
    break;

    case M_BANKRIGHT:
      *cnum = CONTROL_XYROT;
      *cval = CONTROL_MAX_VAL;
    break;

    case M_DIVE:
      *cnum = CONTROL_YVEL;
      *cval = CONTROL_MAX_VAL;
    break;

    case M_ROLLRIGHT:
      *cnum = CONTROL_XVEL;
      *cval = -CONTROL_MAX_VAL;
    break;

    case M_ROLLLEFT:
      *cnum = CONTROL_XVEL;
      *cval = CONTROL_MAX_VAL;
    break;

    case M_CLIMBLEFT:
      *cnum = CONTROL_YVEL;
      *cval = -CONTROL_MAX_VAL;
      if (abs(poll_controls[*cnum]) < abs(*cval)) poll_controls[*cnum] = *cval;
      *cnum = CONTROL_XYROT;
      *cval = -CONTROL_MAX_VAL;
    break;

    case M_CLIMBRIGHT:
      *cnum = CONTROL_YVEL;
      *cval = -CONTROL_MAX_VAL;
      if (abs(poll_controls[*cnum]) < abs(*cval)) poll_controls[*cnum] = *cval;
      *cnum = CONTROL_XYROT;
      *cval = CONTROL_MAX_VAL;
    break;

    case M_DIVERIGHT:
      *cnum = CONTROL_YVEL;
      *cval = CONTROL_MAX_VAL;
      if (abs(poll_controls[*cnum]) < abs(*cval)) poll_controls[*cnum] = *cval;
      *cnum = CONTROL_XYROT;
      *cval = CONTROL_MAX_VAL;
    break;

    case M_DIVELEFT:
      *cnum = CONTROL_YVEL;
      *cval = CONTROL_MAX_VAL;
      if (abs(poll_controls[*cnum]) < abs(*cval)) poll_controls[*cnum] = *cval;
      *cnum = CONTROL_XYROT;
      *cval = -CONTROL_MAX_VAL;
    break;
  }

  return *cnum != -1;
}



//always poll these codes; see init_motion_polling() below
static int always_motion_poll[] =
{
  0x7E, //up arrow
  0x7D, //down arrow
  0x7B, //left arrow
  0x7C, //right arrow
  0x59, //keypad home
  0x5B, //keypad up
  0x5C, //keypad pgup
  0x56, //keypad left
  0x57, //keypad 5
  0x58, //keypad right
  0x53, //keypad end
  0x54, //keypad down
  0x55, //keypad pgdn
  0x4C, //keypad enter
  0x24, //enter

  255   //signal end of list
};



void init_motion_polling(void)
{
  int i, j = 0, code;
  uchar used[256];

  //keep track of which codes have already been added
  memset(used, 0, 256);

  //add move keybinds to list of scancodes to poll
  i = 0;
  while (MoveKeybinds[i].code != 255)
  {
    code = MoveKeybinds[i].code & 255;
    if (!used[code]) {used[code] = 1; motion_key_scancodes[j++] = code;}
    i++;
  }

  //add move cyber keybinds to list of scancodes to poll
  i = 0;
  while (MoveCyberKeybinds[i].code != 255)
  {
    code = MoveCyberKeybinds[i].code & 255;
    if (!used[code]) {used[code] = 1; motion_key_scancodes[j++] = code;}
    i++;
  }

  //always poll these codes, so add them if they weren't added already
  i = 0;
  while (always_motion_poll[i] != 255)
  {
    code = always_motion_poll[i];
    if (!used[code]) {used[code] = 1; motion_key_scancodes[j++] = code;}
    i++;
  }

  motion_key_scancodes[j] = KBC_NONE; //signal end of list

  uiSetKeyboardPolling(motion_key_scancodes);
}



void setup_motion_polling(void)
{
  LG_memset(poll_controls, 0, sizeof(poll_controls));
}



void process_motion_keys(void)
{
  physics_set_player_controls(KEYBD_CONTROL_BANK, poll_controls[0], poll_controls[1], poll_controls[2],
                              poll_controls[3], poll_controls[4], poll_controls[5]);
}



uchar motion_keycheck_handler(uiEvent *ev, LGRegion *r, void *data)
{
  uiPollKeyEvent *ke = (uiPollKeyEvent *)ev;

  // KLC - For Mac version, we'll cook our own, since we have the modifier information.
  ushort cooked = ke->scancode | ke->mods;

  short cnum, cval;

  int moveOK = TRUE;

  if (gKeypadOverride) // if a keypad is showing
  {
    if (ke->scancode >= 0x52 && ke->scancode <= 0x5C) // and a keypad number was entered,
        moveOK = FALSE;                               // don't move.
  }

  if (moveOK)
  {
    if (global_fullmap->cyber && parse_motion_key_cyber(cooked, &cnum, &cval) ||
        parse_motion_key(cooked, &cnum, &cval))
    {
      if (abs(poll_controls[cnum]) < abs(cval))
        poll_controls[cnum] = cval;
    }
  }

  return TRUE;
}
