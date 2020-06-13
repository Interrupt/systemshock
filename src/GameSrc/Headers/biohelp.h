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

#ifndef BIOHELP_H
#define BIOHELP_H

#include "mfdint.h"

errtype biohelp_load_cursor();

errtype mfd_biohelp_init(MFD_Func *f);
void mfd_biohelp_expose(MFD *mfd, ubyte control);
uchar mfd_biohelp_handler(MFD *m, uiEvent *e);

#endif
