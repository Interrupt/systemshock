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
// Since anim is taking top 6 bits of the bitmap_3d, we are
// left with the bottom 10.

#define TPOLY_TYPE_ALT_TMAP    0u
#define TPOLY_TYPE_CUSTOM_MAT  1u
#define TPOLY_TYPE_TEXT_BITMAP 2u
#define TPOLY_TYPE_SCROLL_TEXT 3u

// Note: we may need to steal yet another bit for accessing anims
// 1 bit of scale
// 2 bits of type -- 0 = "screen" anim   1= custom texture material 2 = screen text  3 = scrolling screen
// 7 bits of index
#define TPOLY_INDEX_BITS  7u
#define TPOLY_INDEX_MASK  0x007Fu
#define TPOLY_TYPE_BITS   2u
#define TPOLY_TYPE_MASK   0x0180u
#define TPOLY_SCALE_BITS  2u
#define TPOLY_SCALE_SHIFT 1u
#define TPOLY_SCALE_MASK  0x0600u
#define TPOLY_STYLE_MASK  0x0800u
#define TPOLY_STYLE_BITS  1u

#define RANDOM_TEXT_MAGIC_COOKIE 0x7F

// "texture" 0x77 is algorithmic static, generated each frame
// "texture" 0x76 is like 0x77, but has a change of turning to a SHODAN sometimes when you are near it

#define REGULAR_STATIC_MAGIC_COOKIE 0x77u
#define SHODAN_STATIC_MAGIC_COOKIE  0x76u

// automap is "textures" 0x70 through 0x76
#define NUM_AUTOMAP_MAGIC_COOKIES  6
#define FIRST_AUTOMAP_MAGIC_COOKIE 0x70
