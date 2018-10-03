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
