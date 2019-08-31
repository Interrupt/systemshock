/*

Copyright (C) 2019 Shockolate Project

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

#ifndef TICKCOUNT_H_
#define TICKCOUNT_H_

#include <SDL.h>
#include <stdlib.h>

// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Toolbox/Toolbox-80.html
// number of ticks since system start (1 Tick is about 1/60 second)
uint32_t TickCount(void);

#endif // TICKCOUNT_H_
