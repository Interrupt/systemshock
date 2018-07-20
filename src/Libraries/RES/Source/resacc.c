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
//		ResAcc.c		Resource access
//		Rex E. Bradford
/*
 * $Header: r:/prj/lib/src/res/rcs/resacc.c 1.4 1994/08/30 15:18:20 rex Exp $
 * $Log: resacc.c $
 * Revision 1.4  1994/08/30  15:18:20  rex
 * Made sure ResGet() returns NULL if ResLoadResource() did
 *
 * Revision 1.3  1994/08/30  15:14:32  rex
 * Put in check for NULL return from ResLoadResource
 *
 * Revision 1.2  1994/06/16  11:06:05  rex
 * Modified routines to handle LRU list better (keep locked and nodrop stuff
 * out)
 *
 * Revision 1.1  1994/02/17  11:23:31  rex
 * Initial revision
 *
 */

#include "res.h"
#include "res_.h"
#include "lg.h"
#include <stdlib.h> // free()
#include <string.h>

//	---------------------------------------------------------
//
//	ResLock() locks a resource and returns ptr.
//
//		id = resource id
//
//	Returns: ptr to locked resource
//	---------------------------------------------------------

void *ResLock(Id id) {
    ResDesc *prd;

    //	Check if valid id
    //	DBG(DSRC_RES_ChkIdRef, {if (!ResCheckId(id)) return NULL;});


    prd = RESDESC(id);

    // CC: If already loaded, use the existing bytes
    if (prd->ptr != NULL) {
        prd->lock++;
        return prd->ptr;
    }

    // If resource not loaded, load it now
    if (ResLoadResource(id) == NULL) {
        ERROR("ResLock: Could not load %x", id);
        return (NULL);
    } else if (prd->lock == 0)
        ResRemoveFromLRU(prd);

    prd->lock++;

    // Return ptr
    return (prd->ptr);
}

//	---------------------------------------------------------
//
//	ResUnlock() unlocks a resource.
//
//		id = resource id
//	---------------------------------------------------------
void ResUnlock(Id id) {
    ResDesc *prd;

    // Check if valid id
    if (!ResCheckId(id))
        return;

    // Check for under-lock
    prd = RESDESC(id);

    if (prd->lock == 0) {
        DEBUG("%s: id $%x already unlocked",  __FUNCTION__, id);
        return;
    }

    // Else decrement lock, if 0 move to tail and tally stats
    if (prd->lock > 0)
        prd->lock--;

    if (prd->lock == 0) {
        // CC: Should we free the prd ptr here?
        ResAddToTail(prd);
    }
}

//	-------------------------------------------------------------
//
//	ResGet() gets a ptr to a resource
//
//		id = resource id
//
//	Returns: ptr to resource (ptr only guaranteed until next Malloc(),
//				Lock(), Get(), etc.
//	---------------------------------------------------------

void *ResGet(Id id) {
    ResDesc *prd;

    // Check if valid id
    // ValidateRes(id);

    if (!ResCheckId(id))
        return NULL;

    // Load resource or move to tail
    prd = RESDESC(id);
    if (prd->ptr == NULL) {
        if (ResLoadResource(id) == NULL) {
            return (NULL);
        }
        ResAddToTail(prd);
    } else if (prd->lock == 0) {
        ResMoveToTail(prd);
    }

    // ValidateRes(id);

    //  Return ptr
    return (prd->ptr);
}

//	---------------------------------------------------------
//
//	ResExtract() extracts a resource from an open resource file.
//
//		id   = id
//		buff = ptr to buffer (use ResSize() to compute needed buffer
// size)
//
//	Returns: ptr to supplied buffer, or NULL if problem
//	---------------------------------------------------------
//  For Mac version:  Copies information from resource handle into the buffer.

void *ResExtract(Id id, void *buffer) {
    // Retrieve the data into the buffer, please
    if (ResRetrieve(id, buffer)) {
        return (buffer);
    }

    ERROR("%s: failed for %x", __FUNCTION__, id);
    // If ResRetreive failed, return NULL ptr
    return (NULL);
}

//	----------------------------------------------------------
//
//	ResDrop() drops a resource from memory for awhile.
//
//		id = resource id
//	----------------------------------------------------------
void ResDrop(Id id) {
    ResDesc *prd;

    if (!ResCheckId(id))
        return;

    prd = RESDESC(id);
    if (prd->lock)
        WARN("%s: Block $%x is locked, dropping anyway", __FUNCTION__, id);

    // Remove from LRU chain
    if (prd->lock == 0)
        ResRemoveFromLRU(prd);

    //	Free memory and set ptr to NULL

    if (prd->ptr == NULL) {
        TRACE("%s: Block $%x not in memory, ignoring request\n", __FUNCTION__, id);
        return;
    }

    if (prd->lock != 0) {
        TRACE("%s: Dropping resource 0x%x that's in use.", __FUNCTION__, id);
        prd->lock = 0;
    }

    if (prd->ptr != NULL) {
        free(prd->ptr);
        prd->ptr = NULL;
    }
}

//	-------------------------------------------------------
//
//	ResDelete() deletes a resource forever.
//
//		Id = id of resource
//	-------------------------------------------------------
//  For Mac version:  Call ReleaseResource on the handle and set its ref to
//  null. The next ResLoadResource on the resource will load it back in.

void ResDelete(Id id) {
    ResDesc *prd;

    // If locked, issue warning
    if (!ResCheckId(id))
        return;

    prd = RESDESC(id);

    // If in use: if in ram, free memory & LRU, then in any case zap entry
    if (prd->offset) {
        if (prd->ptr) {
            if (prd->lock == 0)
                ResRemoveFromLRU(prd);
            ResDrop(id);
        }
        memset(prd, 0, sizeof(ResDesc));
    }
}

//	--------------------------------------------------------
//		INTERNAL ROUTINES
//	--------------------------------------------------------
//
//	ResCheckId() checks if id valid.
//
//		id = id to be checked
//
//	Returns: TRUE if id ok, FALSE if invalid & prints warning

bool ResCheckId(Id id) {
    if (id < ID_MIN) {
        DEBUG("%s: id $%x invalid", __FUNCTION__, id);
        return false;
    }
    if (id > resDescMax) {
        DEBUG("%s: id $%x exceeds table", __FUNCTION__, id);
        return false;
    }
    return true;
}
