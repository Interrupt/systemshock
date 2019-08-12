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
//              RefAcc.c                Resource reference access
//              Rex E. Bradford
/*
 * $Header: r:/prj/lib/src/res/rcs/refacc.c 1.4 1994/08/30 15:18:38 rex Exp $
 * $Log: refacc.c $
 * Revision 1.4  1994/08/30  15:18:38  rex
 * Made sure RefGet() returns NULL if ResLoadResource() did
 *
 * Revision 1.3  1994/08/30  15:15:22  rex
 * Put in check for NULL return from ResLoadResource
 *
 * Revision 1.2  1994/06/16  11:05:17  rex
 * Modified RefGet() to handle LRU list better (keep locked guys out)
 *
 * Revision 1.1  1994/02/17  11:23:16  rex
 * Initial revision
 *
 */
#include <assert.h>
#include <stdlib.h> // malloc
#include <unistd.h>

//#include <string.h>
//#include <io.h>
#include "lzw.h"
#include "res.h"
#include "res_.h"

//      ---------------------------------------------------------
//
//      RefLock() locks a compound resource and returns ptr to item.
//
//              ref = resource reference
//
//      Returns: ptr to item within locked compound resource.
//      ---------------------------------------------------------
//  For Mac version:  Change 'ptr' refs to 'hdl', lock resource handle and
//  return ptr.

void *RefLock(Ref ref, ResDecodeFunc decoder, UserDecodeData data, ResFreeFunc freer) {
    Id id = REFID(ref);
    ResDesc *prd;
    RefTable *prt;
    RefIndex index;
    assert(freer == NULL); // not supporting custom free functions yet

    if (!RefCheckRef(ref)) {
        ERROR("%s: Bad ref ID!", __FUNCTION__);
        return NULL;
    }

    // Load block if not in RAM
    prd = RESDESC(id);

    if (prd->decoded == NULL) {
        if (ResLoadResource(id, ResDecodeRefTable, 0, ResFreeRefTable) == NULL) {
            return (NULL);
        }
    }

    if (prd->lock == 0)
        ResRemoveFromLRU(prd);

    if (prd->lock == RES_MAXLOCK)
        prd->lock--;

    prd->lock++;

    // Index into ref table
    prt = (RefTable *)prd->decoded;
    index = REFINDEX(ref);

    // printf("Loading ref %x %x\n", REFID(ref), index);

    // Return ptr
    // We had better loaded the whole thing and not just the reftable.
    assert(prt->raw_data != NULL);
    if (!RefIndexValid(prt, index)) {
        ERROR("%s: Invalid Index %x", __FUNCTION__, ref);
        return (NULL);
    }
    void *raw = ((uint8_t*)(prt->raw_data)) + prt->entries[index].offset;
    if (decoder != NULL) {
	if (prt->entries[index].decoded_data == NULL) {
	    prt->entries[index].decoded_data = decoder(raw, prt->entries[index].size, data);
	}
	return prt->entries[index].decoded_data;
    } else {
        return raw;
    }
}

//      ---------------------------------------------------------
//
//      RefGet() gets a ptr to an item in a compound resource (ref).
//
//              ref = resource reference
//
//      Returns: ptr to item (ptr only guaranteed until next Malloc(),
//                              Lock(), Get(), etc.
//      ---------------------------------------------------------
//  For Mac version:  Lose debug and stats.  Change 'ptr' refs to 'hdl'.  Locks
//  the resource handle before returning the ref ptr.

void *RefGet(Ref ref, ResDecodeFunc decoder, UserDecodeData data, ResFreeFunc freer) {
    Id id = REFID(ref);
    ResDesc *prd;
    RefTable *prt;
    RefIndex index;

    assert(freer == NULL); // custom free not yet supported
    
    // Check for valid ref
    if (RefCheckRef(ref) != true) {
        ERROR("%s: No valid ref!", __FUNCTION__);
        return NULL;
    }

    // Get hold of ref
    prd = RESDESC(id);
    if (prd->ptr == NULL) {
        if (ResLoadResource(REFID(ref), ResDecodeRefTable, 0, ResFreeRefTable) == NULL) {
            ERROR("%s: RefID %x == NULL!", __FUNCTION__, ref);
            return (NULL);
        }
        ResAddToTail(prd);
    } else if (prd->lock == 0) {
        ResMoveToTail(prd);
    }

    // Index into ref table
    prt = (RefTable *)prd->decoded;
    index = REFINDEX(ref);

    // Return ptr
    // We had better loaded the whole thing and not just the reftable.
    assert(prt->raw_data != NULL);
    if (!RefIndexValid(prt, index)) {
        ERROR("%s: Invalid Index %x", __FUNCTION__, ref);
        return (NULL);
    }
    void *raw = ((uint8_t*)(prt->raw_data)) + prt->entries[index].offset;
    if (decoder != NULL) {
	if (prt->entries[index].decoded_data == NULL) {
	    prt->entries[index].decoded_data = decoder(raw, prt->entries[index].size, data);
	}
	return prt->entries[index].decoded_data;
    } else {
        return raw;
    }
}

// Read ref table entries (only; no data) from file. File pointer is expected to
// point at the offsets. prt must have the requisite size and have its 'numRefs'
// field set.
void readRefTableEntries(RefTable *prt, FILE *fd) {
    // Temporary buffer for raw offsets.
    // FIXME assumes little-endian architecture locally.
    const size_t sizeOffsets = (prt->numRefs + 1) * sizeof(int32_t);
    uint32_t* offsets = malloc(sizeOffsets);
    int i;
    const size_t offsetBase = sizeof(RefIndex) + sizeOffsets;
    fread(offsets, sizeof(int32_t), prt->numRefs + 1, fd);
    for (i = 0; i < prt->numRefs; ++i) {
        prt->entries[i].size = offsets[i+1] - offsets[i];
        prt->entries[i].offset = offsets[i] - offsetBase;
        prt->entries[i].decoded_data = NULL;
    }
    free(offsets);
}

//      ---------------------------------------------------------
//
//      ResReadRefTable() reads a compound resource's ref table.
//
//              id = id of compound resource
//
//      Returns: ptr to reftable allocated with Malloc(), or NULL
//      ---------------------------------------------------------
//  For Mac version:  Use "ReadPartialResource" to mimic this code's
//  functionality.

RefTable *ResReadRefTable(Id id) {
    ResDesc *prd;
    RefIndex numRefs;
    RefTable *prt;
    FILE *fd;

    if (!ResCheckId(id))
        return (NULL);

    prd = RESDESC(id);
    fd = resFile[prd->filenum].fd;

    if (fd == NULL) {
        ERROR("%s: id $%x doesn't exist", __FUNCTION__, id);
        return (NULL);
    }

    if (ResIsCompound(id) == 0) {
        ERROR("%s: id $%x is not compound", __FUNCTION__, id);
        return (NULL);
    }

    // Seek to data, read numrefs, allocate table, read in offsets

    fseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
    fread(&numRefs, sizeof(RefIndex), 1, fd);
    prt = malloc(REFTABLESIZE(numRefs));
    prt->numRefs = numRefs;
    prt->raw_data = NULL;
    readRefTableEntries(prt, fd);

    return (prt);
}

//      ---------------------------------------------------------
//
//      ResExtractRefTable() extracts a compound res's ref table.
//
//              id   = id of compound resource
//              prt  = ptr to ref table
//              size = size of ref table in bytes
//
//      Returns: 0 if ok, -1 if error

int32_t ResExtractRefTable(Id id, RefTable *prt, int32_t size) {
    ResDesc *prd;
    FILE *fd;

    // Check id and file number and make sure compound
    if (!ResCheckId(id))
        return (-1);

    prd = RESDESC(id);
    fd = resFile[prd->filenum].fd;
    if (fd == NULL) {
        ERROR("%s: id $%x doesn't exist", __FUNCTION__, id);
        return (-1);
    }
    if (ResIsCompound(id) == 0) {
        ERROR("%s: id $%x is not compound", __FUNCTION__, id);
        return (-1);
    }

    // Seek to data, read numrefs, check table size, read in offsets
    fseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
    fread(&prt->numRefs, sizeof(RefIndex), 1, fd);
    if (REFTABLESIZE(prt->numRefs) > size) {
        ERROR("%s: ref table too large for buffer", __FUNCTION__);
        return (-1);
    }
    readRefTableEntries(prt, fd);

    return (0);
}

void *ResDecodeRefTable(void *raw, size_t size, UserDecodeData data) {
    RefIndex i;
    uint32_t offset;
    // First grab the table size. We'll be pulling stuff in bytewise because it
    // doesn't hurt to proof the code against alignment issues on less lenient
    // processors than x86, so we'll correct endianness while we're at it.
    uint8_t *rp = raw;
    uint16_t numRefs = (uint16_t)*rp | ((uint16_t)rp[1] << 8);
    // Offset to first item in raw data.
    size_t startOffset = sizeof(RefIndex) + (numRefs+1) * sizeof(uint32_t);
    rp += 2;
    // Allocate a directory for it.
    RefTable *prt = malloc(REFTABLESIZE(numRefs));
    prt->numRefs = numRefs;
    // Point the raw data at the original resource in the first instance.
    prt->raw_data = (uint8_t*)raw + startOffset;
    offset = (uint32_t)*rp | ((uint32_t)rp[1] << 8) | ((uint32_t)rp[2] << 16) |
	((uint32_t)rp[3] << 24);
    rp += 4;
    for (i = 0; i < numRefs; ++i) {
	uint32_t next = (uint32_t)*rp | ((uint32_t)rp[1] << 8) |
	    ((uint32_t)rp[2] << 16) | ((uint32_t)rp[3] << 24);
	rp += 4;
	prt->entries[i].size = next - offset;
	prt->entries[i].offset = offset - startOffset;
	prt->entries[i].decoded_data = NULL;
	offset = next;	
    }
    return prt;
}

void ResFreeRefTable(void *ptr) {
    RefTable *prt = ptr;
    RefIndex i;
    for (i = 0; i < prt->numRefs; ++i) {
	free(prt->entries[i].decoded_data);
    }
    free(prt);
}

//      ---------------------------------------------------------
//
// return number of refs, or -1 if error
//
//      ---------------------------------------------------------
int32_t ResNumRefs(Id id) {
    ResDesc *prd;

    // Check id and file number and make sure compound
    if (!ResCheckId(id))
        return (-1);
    if (ResIsCompound(id) == 0) {
        ERROR("%s: id $%x is not compound", __FUNCTION__, id);
        return (-1);
    }
    prd = RESDESC(id);
    if (prd->ptr != NULL) {
        return ((RefTable *)prd->ptr)->numRefs;
    } else {
        FILE *fd = resFile[prd->filenum].fd;
        RefIndex result;
        if (fd == NULL) {
            ERROR("%s: id $%x doesn't exist", __FUNCTION__, id);
            return (-1);
        }
        fseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
        fread(&result, sizeof(RefIndex), 1, fd);
        return result;
    }
}

//      ---------------------------------------------------------
//
//      RefExtract() extracts a ref item from a compound resource.
//
//              prt  = ptr to ref table
//              ref  = ref
//              buff = ptr to buffer (use RefSize() to compute needed buffer
// size)
//
//      Returns: ptr to supplied buffer, or NULL if problem
//      ---------------------------------------------------------

void *RefExtract(RefTable *prt, Ref ref, void *buff) {
    RefIndex index;
    ResDesc *prd;
    FILE *fd;
    int32_t refsize;
    RefIndex numrefs;
    int32_t offset;

    // Check id, get file number
    prd = RESDESC(REFID(ref));
    fd = resFile[prd->filenum].fd;
    index = REFINDEX(ref);

    // get reftable date from rt or by seeking.
    if (prt != NULL) {
        refsize = RefSize(prt, index);
        numrefs = prt->numRefs;
        offset = prt->entries[index].offset + sizeof(RefIndex) + (numrefs+1) * sizeof(uint32_t);
    } else {
        // seek into the file and find the stuff.
        fseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
        fread(&numrefs, sizeof(RefIndex), 1, fd);
        fseek(fd, index * sizeof(int32_t), SEEK_CUR);
        fread(&offset, sizeof(int32_t), 1, fd);
        fread(&refsize, sizeof(int32_t), 1, fd);
        refsize -= offset;
    }
    // Ref table size on disc.
    const size_t rtsize = sizeof(RefIndex) + (numrefs+1) * sizeof(uint32_t);

    prd = RESDESC(REFID(ref));
    fd = resFile[prd->filenum].fd;

    // Seek to start of all data in compound resource
    fseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + rtsize, SEEK_SET);

    // If LZW, extract with skipping, else seek & read
    if (ResCompressed(REFID(ref))) {
        LzwExpandFp2Buff(fd, buff,
                         offset - rtsize, // skip amt
                         refsize);                       // data amt
    } else {
        fseek(fd, offset - rtsize, SEEK_CUR);
        fread(buff, refsize, 1, fd);
    }

    return (buff);
}

void *RefExtractDecoded(RefTable *prt, Ref ref, const ResLayout *layout, void *buff)
{
    // Extract the raw data into a temporary buffer. We could enact some buffer
    // or cache scheme to mitigate memory allocations, but it is $CURRENTYEAR
    // and we don't have the memory or speed constraints of the original DOS
    // game any more.
    uchar *raw = malloc(RefSize(prt, REFINDEX(ref)));
    uchar *rp = raw;
    uchar *b = (uchar *)buff;
    uchar *bp;
    const ResField *field = layout->fields;
    
    RefExtract(prt, ref, raw);
    while (field->type != RFFT_END) {
        bp = b + field->offset;
        switch (field->type) {
        case RFFT_PAD:
            rp += field->offset;
            break;
        case RFFT_UINT8:
            *bp = *rp++;
            break;
        case RFFT_UINT16:
            *(uint16_t*)bp = (uint16_t)rp[0] | ((uint16_t)rp[1] << 8);
            rp += 2;
            break;
        case RFFT_UINT32:
            *(uint32_t*)bp = (uint32_t)rp[0] | ((uint32_t)rp[1] << 8) |
                ((uint32_t)rp[2] << 16) | ((uint32_t)rp[3] << 24);
            rp += 4;
            break;
        case RFFT_RAW: // should be last entry
            memcpy(bp, rp, RefSize(prt,REFINDEX(ref)) - (rp-raw));
            break;
        default:
            assert(!"Invalid resource field type");
        }
        ++field;
    }
    free(raw);
    return buff;
}

/*
int32_t RefInject(RefTable *prt, Ref ref, void *buff)
{
        RefIndex index;
        ResDesc *prd;
        int32_t fd;
   int32_t refsize;
   RefIndex numrefs;
   int32_t offset;

//      Check id, get file number

        if (ResFlags(REFID(ref)) & RDF_LZW)
   {
      return 0;
   }


        prd = RESDESC(REFID(ref));
        fd = resFile[prd->filenum].fd;
        index = REFINDEX(ref);

   // get reftable date from rt or by seeking.
   if (prt != NULL)
   {
      refsize = RefSize(prt,index);
      numrefs = prt->numRefs;
      offset  = prt->offset[index];
   }
   else
   {
      // seek into the file and find the stuff.
        lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
        read(fd, &numrefs, sizeof(RefIndex));
        lseek(fd, index*sizeof(int32_t), SEEK_CUR);
      read(fd,&offset,sizeof(int32_t));
      read(fd,&refsize,sizeof(int32_t));
      refsize -= offset;
      Warning(("Null reftable size = %d offset = %d numrefs =
%d\n",refsize,offset,numrefs));
   }
        DBG(DSRC_RES_ChkIdRef, {if (!RefCheckRef(ref)) return(NULL);});
        DBG(DSRC_RES_ChkIdRef, {if (index >= numrefs) { \
                Warning(("RefExtract: ref $%x index too large\n", ref)); \
                return(NULL); \
                }});

//      Add to cumulative stats

        CUMSTATS(REFID(ref),numExtracts);

//      Seek to start of all data in compound resource

        lseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + REFTABLESIZE(numrefs),
                SEEK_SET);


        lseek(fd, offset - REFTABLESIZE(numrefs), SEEK_CUR);
        return write(fd, buff, refsize);

}
*/

//      ---------------------------------------------------------
//              INTERNAL ROUTINES
//      ---------------------------------------------------------
//
//      RefCheckRef() checks if ref valid.
//
//              ref = ref to be checked
//
//      Returns: true if ref ok, false if invalid & prints warning

bool RefCheckRef(Ref ref) {
    Id id;

    id = REFID(ref);
    if (!ResCheckId(id)) {
        WARN("%s: id $%x is bad\n", __FUNCTION__, id);
        return false;
    }

    if (ResIsCompound(id) == 0) {
        WARN("%s: id $%x is not a compound resource", __FUNCTION__, id);
        return false;
    }

    return true;
}
