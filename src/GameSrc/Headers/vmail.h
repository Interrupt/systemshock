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
#ifndef __VMAIL_H
#define __VMAIL_H

/*
 * $Source: r:/prj/cit/src/inc/RCS/vmail.h $
 * $Revision: 1.2 $
 * $Author: minman $
 * $Date: 1994/09/05 06:43:57 $
 *
 * $Log: vmail.h $
 * Revision 1.2  1994/09/05  06:43:57  minman
 * got rid of clear_vmail!
 *
 * Revision 1.1  1994/01/20  03:00:03  minman
 * Initial revision
 *
 *
 */

// Includes

#define SHIELD_VMAIL 0
#define GROVE_VMAIL  1
#define BRIDGE_VMAIL 2

#define VINTRO_X (SCREEN_VIEW_X + 32)
#define VINTRO_Y (SCREEN_VIEW_Y + 6)

#define VINTRO_W 200
#define VINTRO_H 100

#define RES_FRAMES_shield 0xA40
#define RES_FRAMES_grove 0xA42
#define RES_FRAMES_bridge 0xA42 //0xA44
#define RES_FRAMES_laser1 0xA46
#define RES_FRAMES_status 0xA48
#define RES_FRAMES_explode1 0xA4A

#define RES_shield 0xA4C
#define RES_grove 0xA4D
#define RES_bridge 0xA4E
#define RES_laser1 0xA4F
#define RES_status 0xA57
#define RES_explode1 0xA51

#define BEFORE_ANIM_BITMAP 0x04

#define REF_ANIM_vintro 0xa560000
#define RES_FRAMES_vintro 0xA4A

errtype play_vmail(byte vmail_no);

#endif // __VMAIL_H
