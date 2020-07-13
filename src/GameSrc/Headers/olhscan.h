/*

Copyright (C) 2020 Shockolate Project

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

#ifndef OLHSCAN_H
#define OLHSCAN_H

#include "frtypes.h"

void olh_init_single_scan(fauxrend_context **outxt, fauxrend_context *intxt);
void olh_free_scan(void);
void olh_svga_deal(void);
ushort olh_scan_objs(void);


#endif
