//
// System Shock Enhanced Edition
//
// Copyright (C) 2015-2018 Night Dive Studios, LLC.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// DESCRIPTION:
//      4x4 Drawing routines
//		Reverse engineered by Alex Reimann Cunha Lima
//

#ifndef __DRAW4X4_H
#define __DRAW4X4_H

#ifdef __cplusplus
extern "C" {
#endif

void Draw4x4(uchar* p, int width, int height);
void Draw4x4Reset(uchar* colorset, uchar* hufftab);

#ifdef __cplusplus
}
#endif

#endif
