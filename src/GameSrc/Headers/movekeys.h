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
#define MAX_MOVE_KEYBINDS  256

#define CODE_Q         0x0C
#define CODE_W         0x0D
#define CODE_E         0x0E
#define CODE_A         0x00
#define CODE_S         0x01
#define CODE_D         0x02
#define CODE_Z         0x06
#define CODE_X         0x07
#define CODE_C         0x08
#define CODE_R         0x0F
#define CODE_V         0x09
#define CODE_ENTER     0x24
#define CODE_J         0x26
#define CODE_SPACE     0x31
#define CODE_UP        0x7E
#define CODE_LEFT      0x7B
#define CODE_RIGHT     0x7C
#define CODE_DOWN      0x7D
#define CODE_KP_ENTER  0x4C
#define CODE_KP_HOME   0x59
#define CODE_KP_UP     0x5B
#define CODE_KP_PGUP   0x5C
#define CODE_KP_LEFT   0x56
#define CODE_KP_5      0x57
#define CODE_KP_RIGHT  0x58
#define CODE_KP_END    0x53
#define CODE_KP_DOWN   0x54
#define CODE_KP_PGDN   0x55

typedef struct MOVE_KEYBIND_STRUCT {int code, move;} MOVE_KEYBIND;

enum
{
  M_RUNFORWARD,
  M_FORWARD,
  M_FASTTURNLEFT,
  M_TURNLEFT,
  M_FASTTURNRIGHT,
  M_TURNRIGHT,
  M_BACK,
  M_SLIDELEFT,
  M_SLIDERIGHT,
  M_JUMP,
  M_LEANUP,
  M_LEANLEFT,
  M_LEANRIGHT,
  M_LOOKUP,
  M_LOOKDOWN,
  M_RUNLEFT,
  M_RUNRIGHT,
  M_THRUST, //cyber start
  M_CLIMB,
  M_BANKLEFT,
  M_BANKRIGHT,
  M_DIVE,
  M_ROLLRIGHT,
  M_ROLLLEFT,
  M_CLIMBLEFT,
  M_CLIMBRIGHT,
  M_DIVERIGHT,
  M_DIVELEFT,

  NUM_MOVES
};

uchar motion_keycheck_handler(uiEvent *ev, LGRegion *r, intptr_t data);
void setup_motion_polling(void);
void process_motion_keys(void);
