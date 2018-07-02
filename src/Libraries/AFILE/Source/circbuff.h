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
//		CIRCBUFF.H		Circular buffer
//		Rex E. Bradford

/*
 * $Source: r:/prj/lib/src/afile/RCS/circbuff.h $
 * $Revision: 1.1 $
 * $Author: rex $
 * $Date: 1994/07/22 13:21:07 $
 * $Log: circbuff.h $
 * Revision 1.1  1994/07/22  13:21:07  rex
 * Initial revision
 *
 */

#ifndef __CIRCBUFF_H
#define __CIRCBUFF_H

#ifndef __TYPES_H
#include "lg_types.h"
#endif

typedef struct {
    uint8_t *buff;    // ptr to circular buffer
    uint8_t *buffEnd; // end of buffer
    uint8_t *pput;    // ptr to put data to
    uint8_t *pget;    // ptr to get data from
} CircBuff;

void CircBuffInit(CircBuff *pcb, uint8_t *buff, int32_t length);
void CircBuffReset(CircBuff *pcb);
uint32_t CircBuffRoom(CircBuff *pcb);
uint32_t CircBuffUsed(CircBuff *pcb);
void CircBuffAdvancePut(CircBuff *pcb, int32_t amt);
void CircBuffAdvanceGet(CircBuff *pcb, int32_t amt);
uint8_t CircBuffBetween(uint8_t *ptest, uint8_t *pbeg, uint8_t *pend);

#define CircBuffEmpty(pcb) ((pcb)->pput == (pcb)->pget)
#define CircBuffHitEnd(pcb, p) ((p) >= (pcb)->buffEnd)

#endif
