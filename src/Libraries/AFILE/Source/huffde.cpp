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
//      Huffman decompression routines
//

//============================================================================
// Original header
//		HUFFDE.C		Huffman decompression routines
//		Rex E. Bradford

/*
* $Header: r:/prj/lib/src/dstruct/RCS/huffde.c 1.1 1994/08/22 17:13:01 rex Exp $
* $Log: huffde.c $
 * Revision 1.1  1994/08/22  17:13:01  rex
 * Initial revision
 *
*/

//============================================================================
// Includes
#include <string.h>

#include "lg.h"

//#include "game/libraries/lg/dbg.h"
#include "huff.h"

//============================================================================
// Functions

//	----------------------------------------------------------------
//		DECOMPRESS HUFFMAN MULTI-TABLES
//	----------------------------------------------------------------
//
//	HuffExpandFlashTables() compresses huffman flash-decoder tables.

extern "C" {

void HuffExpandFlashTables(uchar *pFlashTab, uint lenTab, uint *pc,
    int tokSize)
{
    uchar *pft;
    uint token, runCount;
    int runShift;

    //	Setup

    pft = pFlashTab;
    runShift = tokSize * 8;

    //	While still inside dest table, keep going

    while (pft < (pFlashTab + lenTab))
    {

        //	Get next token, extract run count

        token = *pc++;
        runCount = token >> runShift;

        //	Copy that many times into dest

        while (runCount-- != 0)
        {
            memcpy(pft, &token, tokSize);
            pft += tokSize;
        }
    }
}

}
