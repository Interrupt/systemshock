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
#ifndef __WARES_H
#define __WARES_H

#include "gamesys.h"

/*
 * $Source: r:/prj/cit/src/inc/RCS/wares.h $
 * $Revision: 1.27 $
 * $Author: xemu $
 * $Date: 1994/08/07 21:00:48 $
 *
 *
 */

// Includes

// ------
// Macros
// ------

#define WareActive(status) ((status)&WARE_ON)

// -------
// Defines
// -------

#define WARE_HARD         0
#define WARE_SOFT_COMBAT  1
#define WARE_SOFT_DEFENSE 2
#define WARE_SOFT_MISC    3
#define NUM_WARE_TYPES    4

#define WARE_UPDATE_FREQ 280 // How often all wares updated

#define WARE_ON      0x01u // Player_struct status flags
#define WARE_DAMAGED 0x02u
#define WARE_FLASH   0x04u

#define WARE_FLAGS_NONE 0x00 // Here will go various flag bits

// ----------
// Ware Types
// ----------

// HARDWARE

#define FIRST_GOGGLE_WARE 0
#define LAST_GOGGLE_WARE  4

#define HARDWARE_GOGGLE_INFRARED 0
#define HARDWARE_TARGET          1
#define HARDWARE_360             2
#define HARDWARE_AIM             3
#define HARDWARE_HUD             4
#define HARDWARE_BIOWARE         5
#define HARDWARE_AUTOMAP         6
#define HARDWARE_SHIELD          7
#define HARDWARE_EMAIL           8
#define HARDWARE_LANTERN         9
#define HARDWARE_FULLSCREEN     10
#define HARDWARE_ENVIROSUIT     11
#define HARDWARE_MOTION         12
#define HARDWARE_SKATES         13
#define HARDWARE_STATUS         14

// Wacky ware-specific defines
#define LAMP_MASK 0xC0u
#define LAMP_SHF 6u
#define LAMP_SETTING(status) (((status)&LAMP_MASK) >> LAMP_SHF)
#define LAMP_SETTING_SET(status, val) ((status) = (((status) & ~LAMP_MASK) | ((val) << LAMP_SHF)))
#define LAMP_VERSIONS 3
#define SHIELD_SETTING LAMP_SETTING
#define SHIELD_SETTING_SET LAMP_SETTING_SET
#define SHIELD_VERSIONS 4

// ----------
// Structures
// ----------

typedef struct {
    ubyte flags;                                     // Do we have a sideicon, etc.
    ubyte sideicon;                                  // Which sideicon corresponds
    void (*turnon)(uchar visible, uchar real_start); // Function slots for turn on, etc.
    void (*effect)();
    void (*turnoff)(uchar visible, uchar real_stop);
    bool (*check)();
} WARE;

typedef struct {
    ushort timestamp;
    ushort type;
    byte light_value;
    ubyte previous; // was the light on before ??
    ubyte filler;
} LightSchedEvent;

// ----------
// Prototypes
// ----------

void get_ware_pointers(int type, ubyte **player_wares, ubyte **player_status, WARE **wares, int *n);
// Sets several pointers as appropriate to a ware type: the approp. player_struct
// arrays, the approp. global wares property array, and the number of different
// wares for that type

char *get_ware_name(int waretype, int num, char *buf, int bufsz);
// Fills the buffer with the SHORT name of the ware, specified by
// one of the of the four ware types (hard,combat,def,misc),
// and "subtype"

int get_ware_triple(int waretype, int num);
// converts a (waretype,num) pair into a triple.

void use_ware(int waretype, int num);
// Uses a ware from the player's inventory, same format

int get_player_ware_version(int waretype, int num);
// get_player_ware_version returns the version number
// of a ware in the player's inventory.  zero means
// the player doesn't have it.

void wares_init();
// sets up the wares system

void wares_update();
// called from the game loop

void hardware_closedown(uchar visible);
void hardware_startup(uchar visible);
void hardware_power_outage(void);
uchar is_passive_hardware(int n);

bool is_oneshot_misc_software(int n);
int energy_cost(int warenum);

//-----------------------
//   CYBERSPACE ONESHOTS
//-----------------------
void do_turbo_stuff(uchar from_drug);
void turbo_turnon(uchar visible, uchar real_start);
void turbo_turnoff(uchar visible, uchar real_start);
void fakeid_turnon(uchar visible, uchar real_start);
void decoy_turnon(uchar visible, uchar real_start);
void decoy_turnoff(uchar visible, uchar real_stop);
void recall_turnon(uchar visible, uchar real_start);

// ---------------------
//    JUMP JET WARE
// ---------------------
void activate_jumpjets(fix *xcntl, fix *ycntl, fix *zcntl);

// ---------------
//  LANTERN WARE
// ---------------
void lamp_set_vals(void);
void lamp_set_vals_with_offset(byte offset);
void lamp_turnon(uchar visible, uchar real_start);
void lamp_change_setting(byte offset);
void lamp_turnoff(uchar visible, uchar real_stop);
uchar lantern_change_setting_hkey(ushort keycode, uint32_t context, intptr_t data);

//--------------------------
// SHIELD WARE
//--------------------------
void shield_set_absorb(void);
void shield_toggle(uchar visible, uchar real);
uchar shield_change_setting_hkey(ushort keycode, uint32_t context, intptr_t data);

// -------
// Globals
// -------

// what mode are we using.
extern ubyte motionware_mode;
#define MOTION_INACTIVE 0
#define MOTION_SKATES   1
#define MOTION_BOOST    2
#define MOTION_JUMP     3

#endif // __WARES_H
