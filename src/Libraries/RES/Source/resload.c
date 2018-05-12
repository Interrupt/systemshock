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
 * $
 * $Log: resload.c $
 * Revision 1.5  1994/06/16  11:07:44  rex
 * Took LRU list adding out of ResLoadResource()
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

#include "lzw.h"
#include "res.h"
#include "res_.h"

//-------------------------------
//  Private Prototypes
//-------------------------------
void LoadCompressedResource(ResDesc *prd, Id id);

//	-----------------------------------------------------------
//
//	ResLoadResource() loads a resource object, decompressing it if it is
//		compressed.
//
//		id = resource id
//	-----------------------------------------------------------
//  For Mac version:  Call Resource Mgr call LoadResource.

void *ResLoadResource(Id id) {
  ResDesc *prd = RESDESC(id);
  ResDesc2 *prd2 = RESDESC2(id);

  // If doesn't exit, forget it

  // DBG(DSRC_RES_ChkIdRef, {
  if (!ResInUse(id))
    return NULL;
  //});

  // DBG(DSRC_RES_ChkIdRef, {
  if (!ResCheckId(id))
    return NULL;
  //});

  // Spew(DSRC_RES_Read, ("ResLoadResource: loading $%x\n", id));

  // Allocate memory, setting magic id so pager can tell who it is if need be.
  prd->ptr = malloc(prd->size);
  if (prd->ptr == NULL)
    return (NULL);

  // Tally memory allocated to resources

  // DBG(DSRC_RES_Stat, {resStat.totMemAlloc += prd->size;});
  // Spew(DSRC_RES_Stat, ("ResLoadResource: loading id: $%x, alloc %d, total
  // now %d bytes\n", id, prd->size, resStat.totMemAlloc));

  // Add to cumulative stats
  // CUMSTATS(id,numLoads);

  // Load from disk
  ResRetrieve(id, prd->ptr);

  // Tally stats
  // DBG(DSRC_RES_Stat, {resStat.numLoaded++;});

  // Return handle
  return (prd->ptr);
}

//	---------------------------------------------------------
//  Load a compressed resource.  It's different enough for the Mac version to
//  warrant its own function.
//
//  In order to keep prd->hdl as the resource handle (that is, the handle
//  associated with the resource map), we have to be a little non-obvious.
//  First, we'll get the compressed data from the resource file.  Next we copy
//  the compressed data into another handle of the same size. Then we
//  determine how large the expanded data will be, and set prd->hdl to that
//  size. Finally, we expand the data into prd->hdl.
//	---------------------------------------------------------

/*void LoadCompressedResource(ResDesc *prd, Id id) {
  //  Handle mirrorHdl;
  Ptr resPtr, expPtr;
  int32_t exlen;
  int32_t tableSize = 0;
  uint16_t numRefs;

//  DebugString("LoadCompressedResource");
*/ /*  ResDesc2 *prd2 = RESDESC2(id);

  // If everything's still in memory, there's no need to load.
  if (prd->ptr != NULL)
    return;

  // Get the compressed resource from disk.
  prd->ptr = GetResource(resTypeNames[prd2->type], id);
  if (prd->ptr == NULL) {
    DebugStr("LoadCompressedResource: Can't get compressed resource.\n");
    return;
  }

  // Copy the compressed info into the mirror handle.
  exlen = GetHandleSize(prd->ptr);
  mirrorHdl = NewHandle(exlen);
  if (mirrorHdl == NULL) {
    DebugStr("LoadCompressedResource: Can't allocate mirror handle.\n");
    return;
  }
  BlockMoveData(*prd->ptr, *mirrorHdl, exlen);

  // Point to the beginning of the compressed data in the mirror handle.
  // HLock(mirrorHdl);
  resPtr = *mirrorHdl;

  // Determine the expanded buffer size.
  if (prd2->flags & RDF_COMPOUND) {
    numRefs = *(int16_t *)resPtr;
    tableSize = REFTABLESIZE(numRefs);
    resPtr += tableSize;
  }
  exlen = LzwExpandBuff2Null(resPtr, 0, 0);

  // Set prd->hdl to the expanded buffer size.
  SetHandleSize(prd->ptr, exlen + tableSize);
  if (MemError()) {
    DebugStr(
        "LoadCompressedResource: Can't resize resource handle for expansion.");
    return;
  }

  // Point to the beginning of the expanded data handle.
  // HLock(prd->hdl);
  expPtr = *(prd->hdl);

  // Expand-o-rama!  If a compound resource, copy the uncompressed refTable
  // over first.
  if (prd2->flags & RDF_COMPOUND) {
    BlockMoveData(resPtr - tableSize, expPtr, tableSize);
    expPtr += tableSize;
  }
  exlen = LzwExpandBuff2Buff(resPtr, expPtr, 0, 0);
  if (exlen < 0) {
    DebugStr("LoadCompressedResource: Can't expand resource.\n");
    return;
  }*/ /*

   // DisposeHandle(mirrorHdl); // Free the mirror buffer.
 }*/

//	---------------------------------------------------------
//
//	ResRetrieve() retrieves a resource from disk.
//
//		id     = id of resource
//		buffer = ptr to buffer to load into (must be big enough)
//
//	Returns: TRUE if retrieved, FALSE if problem

uint8_t ResRetrieve(Id id, void *buffer) {
  ResDesc *prd;
  ResDesc2 *prd2;
  FILE *fd;
  uint8_t *p;
  int32_t size;
  RefIndex numRefs;

  // Check id and file number
  // DBG(DSRC_RES_ChkIdRef, {if (!ResCheckId(id)) return false;});

  prd = RESDESC(id);
  prd2 = RESDESC2(id);
  fd = resFile[prd->filenum].fd;
  // DBG(DSRC_RES_ChkIdRef, {if (fd < 0) { \
//		Warning(("ResRetrieve: id $%x doesn't exist\n", id)); \
//		return false; \
//		}});

  // Seek to data, set up
  fseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
  p = buffer;
  size = prd->size;

  // If compound, read in ref table
  if (prd2->flags & RDF_COMPOUND) {
    fread(p, sizeof(int16_t), 1, fd);
    numRefs = *(int16_t *)p;
    p += sizeof(int16_t);
    fread(p, sizeof(int32_t) * (numRefs + 1), 1, fd);
    p += sizeof(int32_t) * (numRefs + 1);
    size -= REFTABLESIZE(numRefs);
  }

  // Read in data
  if (prd2->flags & RDF_LZW)
    LzwExpandFd2Buff(fd, p, 0, 0);
  else
    fread(p, size, 1, fd);

  return true;
}
