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
// Rsd unpacking into a bitmap where row=width.
//
// 68K and PowerPC versions
//

#include "grs.h"
#include "rsdunpck.h"
#include <stdbool.h>

//----------------------------------------------------------------------------
// PowerPC version
#define kMinLongLoop 4			// minimum # of bytes to need before using long store loop

uchar *gr_rsd8_unpack(uchar *src, uchar *dest)
 {
 	uchar		code,val;
 	short		count,count2;
 	ushort	longcode;
 	ulong		longval, *longdest, *longsrc;
 
 	do
 	 {
 		code = *(src++);
 		if (!code) // run of bytes
 		 {	
 		 	count = *(src++); // get count
 		 	val = *(src++); 	// get val
 		 	
 		 	if (count>=kMinLongLoop)		// if at least kMinLongLoop bytes, do long word stuff
 		 	 {
 		 	 	longval = val + (((ulong) val)<<8);
 		 	 	longval += longval<<16;
 		 	 	count2 = count>>2;
 		 	 	count &= 3;
 		 	 	longdest = (ulong *) dest;
 		 	 	
 		 	 	while (count2--)
 		 	 	 	*(longdest++) = longval;
 		 	 	dest = (uchar *) longdest;	
 		 	 }
 		 	
 		 	// do rest of bytes
 		 	while (count--)
 		 		*(dest++) = val;
 		 }
 		else if (code<0x80) // dump (copy) bytes
 		 {
 		 	count = code;
 		 	if (code>=kMinLongLoop)		// if at least kMinLongLoop bytes, do long word stuff
 		 	 {
 		 	 	count2 = count>>2;
 		 	 	count &= 3;
 		 	 	longdest = (ulong *) dest;
 		 	 	longsrc = (ulong *) src;
 		 	 	
 		 	 	while (count2--)
 		 	 	 	*(longdest++) = *(longsrc++);
 		 	 	dest = (uchar *) longdest;	
 		 	 	src = (uchar *) longsrc;	
 		 	 }
 		 	
 		 	// do rest of bytes
 		 	while (count--)
 		 		*(dest++) = *(src++);
 		 }
 		else if (code>0x80) // skip (zero) bytes)
 		 {
 		 	count = code & 0x007f;	// clear high byte
 		 	val = longval = 0L;
 		 	
 		 	if (count>=kMinLongLoop)		// if at least kMinLongLoop bytes, do long word stuff
 		 	 {
 		 	 	count2 = count>>2;
 		 	 	count &= 3;
 		 	 	longdest = (ulong *) dest;
 		 	 	
 		 	 	while (count2--)
 		 	 	 	*(longdest++) = longval;
 		 	 	dest = (uchar *) longdest;	
 		 	 }
 		 	
 		 	// do rest of bytes
 		 	while (count--)
 		 		*(dest++) = val;
 		 }
 		else	// long opcode 
 		 {
 		 	longcode = * (ushort *) src; 		 	
 		 	src += 2L;
 		 	
 		 	if (!longcode) break;		// done?
			else if (longcode<0x8000)	// skip (zero)
			 {
	 		 	count = longcode;
	 		 	val = longval = 0L;
	 		 	
	 		 	if (count>=kMinLongLoop)		// if at least kMinLongLoop bytes, do long word stuff
	 		 	 {
	 		 	 	count2 = count>>2;
	 		 	 	count &= 3;
	 		 	 	longdest = (ulong *) dest;
	 		 	 	
	 		 	 	while (count2--)
	 		 	 	 	*(longdest++) = longval;
	 		 	 	dest = (uchar *) longdest;	
	 		 	 }
	 		 	
	 		 	// do rest of bytes
	 		 	while (count--)
	 		 		*(dest++) = val;
			 }
			else if (longcode<0xC000)	// dump (copy)
			 {
			 	count = longcode & 0x7fff;	// clear high bit
			 	
	 		 	if (count>=kMinLongLoop)		// if at least kMinLongLoop bytes, do long word stuff
	 		 	 {
	 		 	 	count2 = count>>2;
	 		 	 	count &= 3;
	 		 	 	longdest = (ulong *) dest;
	 		 	 	longsrc = (ulong *) src;
	 		 	 	
	 		 	 	while (count2--)
	 		 	 	 	*(longdest++) = *(longsrc++);
	 		 	 	dest = (uchar *) longdest;	
	 		 	 	src = (uchar *) longsrc;	
	 		 	 }
	 		 	
	 		 	// do rest of bytes
	 		 	while (count--)
	 		 		*(dest++) = *(src++);
			 }
			else	// run of bytes
			 {
	 		 	count = longcode & 0x3fff;
	 		 	val = *(src++); 	// get val
	 		 	
	 		 	if (count>=kMinLongLoop)		// if at least kMinLongLoop bytes, do long word stuff
	 		 	 {
	 		 	 	longval = val + (((ulong) val)<<8);
	 		 	 	longval += longval<<16;
	 		 	 	count2 = count>>2;
	 		 	 	count &= 3;
	 		 	 	longdest = (ulong *) dest;
	 		 	 	
	 		 	 	while (count2--)
	 		 	 	 	*(longdest++) = longval;
	 		 	 	dest = (uchar *) longdest;	
	 		 	 }
	 		 	
	 		 	// do rest of bytes
	 		 	while (count--)
	 		 		*(dest++) = val;
			 } 		 	
 		 }
 	 } 
 	while (true);
 	
 	return(dest);
 }
