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

#ifndef DIGIFX_H
#define DIGIFX_H

int compute_sfx_vol(ushort x1, ushort y1, ushort x2, ushort y2);
int compute_sfx_pan(ushort x1, ushort y1, ushort x2, ushort y2, fixang our_ang);

uchar set_sample_pan_gain(snd_digi_parms *sdp);

#endif
