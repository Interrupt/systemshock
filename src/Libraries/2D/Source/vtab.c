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
//                                                     
// $Source: r:/prj/lib/src/2d/RCS/vtab.c $
// $Revision: 1.1 $
// $Author: kevin $
// $Date: 1994/07/28 01:23:36 $
//
// Procedure to create temporary vtab.
//
// This file is part of the 2d library.
//

#include "grs.h"
#include "vtab.h"
#include "buffer.h"


// build a table of line starts for the bitmap parameter
long *gr_make_vtab (grs_bitmap *bm)
 {
 	void 	*mem;
 	long	*dest;
 	long	i,add,row;
 	long	maxh;
 	
 	mem = gr_alloc_temp(bm->h<<2);
 	row = bm->row;
	add = 0L;
	maxh = bm->h;
	dest = (long *) mem;
	
	for (i=0; i<maxh; i++)
	 {
	 	*(dest++) = add;
	 	add += row;
	 }

 	return((long *) mem);
 }

