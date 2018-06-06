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
//		ResMake.c		Resource making
//		Rex E. Bradford
/*
 * $Header: n:/project/lib/src/res/rcs/resmake.c 1.2 1994/06/16 11:08:04 rex Exp
 * $
 * $Log: resmake.c $
 * Revision 1.2  1994/06/16  11:08:04  rex
 * Modified LRU list handling, lock resource made with ResMake() instead of
 * setting RDF_NODROP flag
 *
 * Revision 1.1  1994/02/17  11:23:57  rex
 * Initial revision
 *
 */

#include <string.h>
#include <stdlib.h>

#include "res.h"
#include "res_.h"

//	--------------------------------------------------------
//
//	ResMake() makes a resource from a data block.
//
//		Id      = id of resource
//		ptr     = ptr to memory block (resource is not copied; this
//                should point to storage where the resource can live
//                indefinitely)
//		size    = size of resource in bytes
//		type    = resource type (RTYPE_XXX)
//		filenum = file number
//		flags   = flags (RDF_XXX)
//	--------------------------------------------------------
//  For Mac version, use Resource Manager to add the resource to indicated res
//  file.

void ResMake(Id id, void *ptr, int32_t size, uint8_t type, int32_t filenum,
             uint8_t flags) {

  printf("ResMake\n");	

  ResDesc *prd;
  ResDesc2 *prd2;

  ResExtendDesc(id);
  // Check for resource at that id.  If the handle exists, then just change the
  // handle (adjusting for size if needed, of course).

  // Extend res desc table if need to
  prd = RESDESC(id);
  prd2 = RESDESC2(id);
  // If resource has id, delete it
  if (prd->offset) {
    ResDelete(id);
  }

  // Add us to the soup, set lock so doesn't get swapped out
  prd->ptr = ptr;
  prd->size = size;
  prd->filenum = filenum;
  prd->lock = 1;
  prd->offset = RES_OFFSET_PENDING;
  prd2->flags = flags;
  prd2->type = type;
}

//	---------------------------------------------------------------
//
//	ResMakeCompound() makes an empty compound resource
//
//		id      = id of resource
//		type    = resource type (RTYPE_XXX)
//		filenum = file number
//		flags   = flags (RDF_XXX, RDF_COMPOUND automatically added)

void ResMakeCompound(Id id, uint8_t type, int32_t filenum, uint8_t flags) {
  RefTable *prt;
  int32_t sizeTable;

  // Build empty compound resource in allocated memory
  // Spew(DSRC_RES_Make, ("ResMake: making compound resource $%x\n", id));

  sizeTable = REFTABLESIZE(0);
  prt = (RefTable *)malloc(sizeTable);
  prt->numRefs = 0;
  prt->offset[0] = sizeTable;

  // Make a resource out of it
  ResMake(id, prt, sizeTable, type, filenum, flags | RDF_COMPOUND);
}

//	---------------------------------------------------------------
//
//	ResAddRef() adds an item to a compound resource.
//
//		ref      = reference
//		pitem    = ptr to item's data (copied from here, unlike simple
//			resource)
//		itemSize = size of item
//	---------------------------------------------------------------
//  For Mac version:  Change references from 'ptr' to 'hdl'.  Use Mac memory
//  allocating routines.

void ResAddRef(Ref ref, void *pitem, int32_t itemSize) {
  ResDesc *prd;
  RefTable *prt;
  RefIndex index, i;
  int32_t sizeItemOffsets, oldSize, sizeDiff;

  // Error check
  // DBG(DSRC_RES_ChkIdRef, {
  if (!RefCheckRef(ref))
    return;
  //});

  // Get vital info (and get into memory if not already)
  // Spew(DSRC_RES_Make, ("ResAddRef: adding ref $%x\n", ref));

  prd = RESDESC(REFID(ref));

  prt = (RefTable *)prd->ptr;
  if (prt == NULL) {
    prt = (RefTable *)RefGet(ref);
  }

  // If index within current range of compound resource, replace or insert
  index = REFINDEX(ref);
  if (index < prt->numRefs) {
    oldSize = RefSize(prt, index);

    // If same size, just copy in
    if (itemSize == oldSize) {
      // Spew(DSRC_RES_Make, ("ResAddRef: replacing same size ref\n"));
      memcpy(REFPTR(prt, index), pitem, itemSize);
    }
    // Else if new item smaller, reduce offsets, shift data, insert new data
    else if (itemSize < oldSize) {
      // Spew(DSRC_RES_Make, ("ResAddRef: replacing larger ref\n"));
      sizeDiff = oldSize - itemSize;

      for (i = index + 1; i <= prt->numRefs; i++)
        prt->offset[i] -= sizeDiff;
      prd->size -= sizeDiff;
      memmove(REFPTR(prt, index + 1), REFPTR(prt, index + 1) + sizeDiff,
              prt->offset[prt->numRefs] - prt->offset[index + 1]);
      memcpy(REFPTR(prt, index), pitem, itemSize);
      prd->ptr = realloc(prd->ptr, prd->size);
    } else {
      // New item is larger.
      // Spew(DSRC_RES_Make, ("ResAddRef: replacing smaller ref\n"));
      sizeDiff = itemSize - oldSize;

      prd->size += sizeDiff;
      prd->ptr = prt = (RefTable *)realloc(prd->ptr, prd->size);
      memmove(REFPTR(prt, index + 1) + sizeDiff, REFPTR(prt, index + 1),
              prt->offset[prt->numRefs] - prt->offset[index + 1]);

      for (i = index + 1; i <= prt->numRefs; i++)
        prt->offset[i] += sizeDiff;
      memcpy(REFPTR(prt, index), pitem, itemSize);
    }
  } else {
    // Else if index exceeds current range, expand
    // Spew(DSRC_RES_Make, ("ResAddRef: extending compound resource\n"));

    // Extend resource for new offset(s) and data item
    sizeItemOffsets = sizeof(int32_t) * ((index + 1) - prt->numRefs);
    prd->size += sizeItemOffsets + itemSize;
    prd->ptr = realloc(prd->ptr, prd->size);
    prt = (RefTable *)prd->ptr;

    // Shift data upwards to make room for new offset(s)
    memmove(REFPTR(prt, 0) + sizeItemOffsets, REFPTR(prt, 0),
            prd->size - REFTABLESIZE(index + 1));

    // Advance old offsets, set new ones
    for (i = 0; i <= prt->numRefs; i++) {
      prt->offset[i] += sizeItemOffsets;
    }

    for (i = prt->numRefs + 1; i <= index; i++) {
      prt->offset[i] = prt->offset[prt->numRefs];
    }
    // Save size of whole dir entry
    prt->offset[index + 1] = prt->offset[index] + itemSize;

    // Copy data into place, set new numRefs
    memcpy(REFPTR(prt, index), pitem, itemSize);
    prt->numRefs = index + 1;
  }
}

//	-------------------------------------------------------------
//
//	ResUnmake() removes a resource from the LRU list and sets its
//		ptr to NULL.  In this way, a program may take over management
//		of the resource data, and the RES system forgets about it.
//		This is typically done when user-managed data needs to be
//		written to a resource file, using ResMake(), ResWrite(),
//		ResUnmake().
//
//		id = id of resource to unmake
//	--------------------------------------------------------
//  For Mac version: use ReleaseResource to free the handle (the pointer that
//  the handle was made from will still be around).

void ResUnmake(Id id) { memset(RESDESC(id), 0, sizeof(ResDesc)); }