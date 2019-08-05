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
#define SOFTWARE_DRILL     0
#define SOFTWARE_SPEW      1
#define SOFTWARE_MINE      2
#define SOFTWARE_DISC      3
#define SOFTWARE_PULSER    4
#define SOFTWARE_SCRAMBLER 5
#define SOFTWARE_VIRUS     6

#define SOFTWARE_CSHIELD    0
#define SOFTWARE_OLD_FAKEID 1
#define SOFTWARE_ICE        2

#define SOFTWARE_TURBO  0
#define SOFTWARE_FAKEID 1
#define SOFTWARE_DECOY  2
#define SOFTWARE_RECALL 3
#define SOFTWARE_GAMES  4

#define SOFTWARE_FILTER   4
#define SOFTWARE_MONITOR  5
#define SOFTWARE_IDENTIFY 6
#define SOFTWARE_TRACE    7
#define SOFTWARE_TOGGLE   8

// ORing combination
#define GAME_PING       0b00000001u
#define GAME_EEL_ZAPPER 0b00000010u
#define GAME_ROAD       0b00000100u
#define GAME_BOTBOUNCE  0b00001000u
#define GAME_15         0b00010000u
#define GAME_TRIPTOE    0b00100000u
#define GAME_GAME6      0b01000000u
#define GAME_WING0      0b10000000u