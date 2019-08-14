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
//		Res.C		Resource Manager primary access routines
//		Rex E. Bradford (REX)
//
//		See the doc RESOURCE.DOC for information.
/*
 * $Header: r:/prj/lib/src/res/rcs/res.c 1.24 1994/07/15 18:19:33 xemu Exp $
 * $Log: res.c $
 * Revision 1.24  1994/07/15  18:19:33  xemu
 * added ResShrinkResDescTable
 *
 * Revision 1.23  1994/05/26  13:51:55  rex
 * Added ResInstallPager(ResDefaultPager) to ResInit()
 *
 * Revision 1.22  1994/02/17  11:24:51  rex
 * Moved most funcs out into other .c files
 *
 */

//#include <io.h>
//#include <stdlib.h>
#include <string.h>

#include <lg.h>
#include "lzw.h"
#include "res.h"
//#include <memall.h>
//#include <_res.h>

//	The resource descriptor table

ResDesc *gResDesc;   // ptr to array of resource descriptors
ResDesc2 *gResDesc2; // secondary array, shared buff with resdesc
Id resDescMax;       // max id in res desc
// default max resource id
#define DEFAULT_RESMAX 32767
// grow by blocks of 1024 resources must be power of 2!
#define DEFAULT_RESGROW 32768

//	Some variables
/*
ResStat resStat;						// stats held
here static bool resPushedAllocators;	// did we push our allocators?
*/

//	---------------------------------------------------------
//		INITIALIZATION AND TERMINATION
//	---------------------------------------------------------
//
//	ResInit() initializes resource manager.

void ResInit() {
    int32_t i;

    // We must exit cleanly
    atexit(ResTerm);

    // init LZW system
    LzwInit();

    // Allocate initial resource descriptor table, default size (can't fail)
    TRACE("%s: RES system initialization", __FUNCTION__);

    resDescMax = DEFAULT_RESMAX;
    gResDesc = (ResDesc *)calloc(DEFAULT_RESMAX + 1, sizeof(ResDesc) + sizeof(ResDesc2));
    if (gResDesc == NULL)
        ERROR("ResInit: Can't allocate the global resource descriptor table.");
    gResDesc2 = (ResDesc2 *)(gResDesc + (DEFAULT_RESMAX + 1));
    gResDesc[ID_HEAD].prev = 0;
    gResDesc[ID_HEAD].next = ID_TAIL;
    gResDesc[ID_TAIL].prev = ID_HEAD;
    gResDesc[ID_TAIL].next = 0;

    // Clear file descriptor array

    for (i = 0; i <= MAX_RESFILENUM; i++)
        resFile[i].fd = NULL;

    // Add directory pointed to by RES env var to search path

    /*
    p = getenv("RES");
    if (p)
            ResAddPath(p);
    */

    TRACE("%s: RES system initialized", __FUNCTION__);

    // Install default pager
    // ResInstallPager(ResDefaultPager);
}

//	---------------------------------------------------------
//
//	ResTerm() terminates resource manager.

void ResTerm() {
    int32_t i;
    // Close all open resource files
    for (i = 0; i <= MAX_RESFILENUM; i++) {
        if (resFile[i].fd >= 0)
            ResCloseFile(i);
    }

    // Free up resource descriptor table

    if (gResDesc) {
        free(gResDesc);
        gResDesc = NULL;
        gResDesc2 = NULL;
        resDescMax = 0;
    }
    // We're outta here
    TRACE("%s: RES system terminated", __FUNCTION__);
}

//	---------------------------------------------------------
//
//	ResGrowResDescTable() grows resource descriptor table to
//	handle a new id.
//
//	This routine is normally called internally, but a client
//	program may call it directly too.
//
//		id = id

void ResGrowResDescTable(Id id) {
    int32_t newAmt, currAmt;
    ResDesc2 *pNewResDesc2;

    // Calculate size of new table and size of current

    newAmt = (id + DEFAULT_RESGROW) & ~(DEFAULT_RESGROW - 1);
    currAmt = resDescMax + 1;

    // If need to grow, do it, clearing new entries

    if (newAmt > currAmt) {
        WARN("%s: extending to $%x entries", __FUNCTION__, newAmt);

        // Realloc double-array buffer and check for error
        gResDesc = (ResDesc *)realloc(gResDesc, newAmt * (sizeof(ResDesc) + sizeof(ResDesc2)));
        if (gResDesc == NULL) {
            ERROR("%s: RES DESCRIPTOR TABLE BAD!!!", __FUNCTION__);
            return;
        }

        //  Compute new location for gResDesc2[] array at top of buffer,
        //  and move the gResDesc2[] array up there

        gResDesc2 = (ResDesc2 *)(gResDesc + currAmt);
        pNewResDesc2 = (ResDesc2 *)(gResDesc + newAmt);
        memmove(pNewResDesc2, gResDesc2, currAmt * sizeof(ResDesc2));
        gResDesc2 = pNewResDesc2;

        //  Clear extra entries in both tables

        memset(gResDesc + currAmt, 0, (newAmt - currAmt) * sizeof(ResDesc));
        memset(gResDesc2 + currAmt, 0, (newAmt - currAmt) * sizeof(ResDesc2));

        //  Set new max id limit

        resDescMax = newAmt - 1;


    }
}

//	---------------------------------------------------------
//
//	ResShrinkResDescTable() resizes the descriptor table to be
//	the minimum allowable size with the currently in-use resources.
//
/*
void ResShrinkResDescTable()
{
        int32_t newAmt,currAmt;
   // id is the largest used ID
   Id id;

// Calculate largest used ID
   id = resDescMax;
   while ((id > ID_MIN) && (!ResInUse(id)))
      id--;
//   Spew(DSRC_RES_General, ("largest ID in use is %x.\n",id));

//	Calculate size of new table and size of current

        newAmt = (id + DEFAULT_RESGROW) & ~(DEFAULT_RESGROW - 1);
        currAmt = resDescMax + 1;

//	If need to shrink do it
// note that we don't increase the stat table

        if (currAmt > newAmt)
        {
//		Spew(DSRC_RES_General,
//			("ResGrowResDescTable: extending to $%x entries\n",
newAmt));

                SetPtrSize(gResDesc, newAmt * sizeof(ResDesc));
                if (MemError() != noErr)
                {
//���			Warning(("ResGrowDescTable: RES DESCRIPTOR TABLE
BAD!!!\n")); return;
                }
                resDescMax = newAmt - 1;
      }
}
*/
