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
//		ResLoad.c	Load resource from resfile
//		Rex E. Bradford
/*
 * $Header: n:/project/lib/src/res/rcs/resload.c 1.5 1994/06/16 11:07:44 rex Exp
 * $ $Log: resload.c $ Revision 1.5  1994/06/16  11:07:44  rex Took LRU list
 * adding out of ResLoadResource()
 *
 * Revision 1.4  1994/05/26  13:52:32  rex
 * Surrounded Malloc() for loading resource with setting of idBeingLoaded,
 * so installable pager can make use of this.
 *
 * Revision 1.3  1994/04/19  16:40:28  rex
 * Added check for 0-size resource
 *
 * Revision 1.2  1994/03/14  16:10:47  rex
 * Added id to spew in ResLoadResource()
 *
 * Revision 1.1  1994/02/17  11:23:39  rex
 * Initial revision
 *
 */

//#include <io.h>
#include <stdlib.h>

#include "lzw.h"
#include "res.h"
#include "res_.h"
#include "dbg.h"

//-------------------------------
//  Private Prototypes
//-------------------------------

//	-----------------------------------------------------------
//
//	ResLoadResource() loads a resource object, decompressing it if it is
//		compressed.
//
//		id = resource id
//	-----------------------------------------------------------

void *ResLoadResource(Id id) {
    ResDesc *prd;

    // If doesn't exit, forget it
    if (!ResInUse(id))
        return NULL;
    if (!ResCheckId(id))
        return NULL;

    TRACE("%s loading $%x", __FUNCTION__, id);

    prd = RESDESC(id);

    if (prd->size == 0) {
        return NULL;
    }

    // Allocate memory, setting magic id so pager can tell who it is if need be.
    prd->ptr = malloc(prd->size);
    if (prd->ptr == NULL)
        return (NULL);

    // Load from disk
    ResRetrieve(id, prd->ptr);

    // Return ptr
    return (prd->ptr);
}

//	---------------------------------------------------------
//
//	ResRetrieve() retrieves a resource from disk.
//
//		id     = id of resource
//		buffer = ptr to buffer to load into (must be big enough)
//
//	Returns: TRUE if retrieved, FALSE if problem

bool ResRetrieve(Id id, void *buffer) {
    ResDesc *prd;
    ResDesc2 *prd2;
    FILE *fd;
    uint8_t *p;
    int32_t size;
    RefIndex numRefs;

    // Check id and file number
    if (!ResCheckId(id)) {
        TRACE("%s: failed ResCheckId! %x\n", __FUNCTION__, id);
        return false;
    }

    prd = RESDESC(id);
    prd2 = RESDESC2(id);
    fd = resFile[prd->filenum].fd;

    if (fd == NULL) {
        WARN("%s: id $%x doesn't exist", __FUNCTION__, id);
        return false;
    }

    // Seek to data, set up
    fseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
    p = (uint8_t *)buffer;
    size = prd->size;

    // If compound, read in ref table
    if (prd2->flags & RDF_COMPOUND) {
        fread(p, sizeof(int16_t), 1, fd);
        numRefs = *(uint16_t *)p;
        p += sizeof(int16_t);
        fread(p, sizeof(int32_t), (numRefs + 1), fd);
        p += sizeof(int32_t) * (numRefs + 1);
        size -= REFTABLESIZE(numRefs);
    }

    // Read in data
    if (prd2->flags & RDF_LZW) {
        LzwExpandFp2Buff(fd, p, 0, 0);
    } else {
        fread(p, size, 1, fd);
    }

    return true;
}
