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
//		RefAcc.c		Resource reference access
//		Rex E. Bradford
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

#include <stdlib.h>
#include <unistd.h>

#include "lzw.h"
#include "res.h"
#include "res_.h"


//	---------------------------------------------------------
//
//	RefLock() locks a compound resource and returns ptr to item.
//
//		ref = resource reference
//
//	Returns: ptr to item within locked compound resource.
//	---------------------------------------------------------
//  For Mac version:  Change 'ptr' refs to 'hdl', lock resource handle and
//  return ptr.

void *RefLock(Ref ref) {
  ResDesc *prd;
  RefTable *prt;
  RefIndex index;

  //	Check for valid ref

  //	DBG(DSRC_RES_ChkIdRef, {if (!RefCheckRef(ref)) return NULL;});

  //	Add to cumulative stats

  //	CUMSTATS(REFID(ref),numLocks);

  //	Load block if not in RAM

  prd = RESDESC(REFID(ref));
  if (ResLoadResource(REFID(ref)) == NULL)
    return (NULL);
  //	if (prd->lock == 0)
  //		ResRemoveFromLRU(prd);

  //	Tally stats

  //	DBG(DSRC_RES_Stat, {if (prd->lock == 0) resStat.numLocked++;});

  //	Bump lock count

  //	DBG(DSRC_RES_ChkLock, {if (prd->lock == RES_MAXLOCK) prd->lock--;});
  prd->lock++;

  //	Index into ref table

  // if (prd->lock == 1)
  //  HLock(prd->filenum);
  prt = (RefTable *)prd->ptr;
  index = REFINDEX(ref);
  //	DBG(DSRC_RES_ChkIdRef, {if (!RefIndexValid(prt,index)) \
//		Warning(("RefLock: reference: $%x bad, index out of range\n", ref));});

  //	Return ptr

  if (!RefIndexValid(prt, index))
    return (NULL);
  else
    return (((uint8_t *)prt) + (prt->offset[index]));
}

//	---------------------------------------------------------
//
//	RefGet() gets a ptr to an item in a compound resource (ref).
//
//		ref = resource reference
//
//	Returns: ptr to item (ptr only guaranteed until next Malloc(),
//				Lock(), Get(), etc.
//	---------------------------------------------------------
//  For Mac version:  Lose debug and stats.  Change 'ptr' refs to 'hdl'.  Locks
//  the
//  resource handle before returning the ref ptr.

void *RefGet(Ref ref) {
  Id id;
  ResDesc *prd;
  RefTable *prt;
  RefIndex index;

  id = REFID(ref);



  //	Check for valid ref

  //	DBG(DSRC_RES_ChkIdRef, {if (!RefCheckRef(ref)) return NULL;});

  //	Add to cumulative stats

  //	CUMSTATS(REFID(ref),numGets);

  //	Get hold of ref

  prd = RESDESC(id);

  prt = (RefTable *) prd->ptr;
  index = REFINDEX(ref);


  if (ResLoadResource(REFID(ref)) == NULL)
    return (NULL);
  //		ResAddToTail(prd);
  //	else if (prd->lock == 0)
  //		ResMoveToTail(prd);

  //	Index into ref table

  // HLock(prd->filenum);
  prt = (RefTable *)prd->filenum;
  index = REFINDEX(ref);
  //	DBG(DSRC_RES_ChkIdRef, {if (!RefIndexValid(prt,index)) \
//		Warning(("RefGet: reference: $%x bad, index out of range\n", ref));});

  //	Return ptr

    return (((uint8_t *)prt) + (prt->offset[index]));
}

//	---------------------------------------------------------
//
//	ResReadRefTable() reads a compound resource's ref table.
//
//		id = id of compound resource
//
//	Returns: ptr to reftable allocated with Malloc(), or NULL
//	---------------------------------------------------------
//  For Mac version:  Use "ReadPartialResource" to mimic this code's
//  functionality.

RefTable *ResReadRefTable(Id id) {
  FILE *fd;
  ResDesc *prd;
  // Handle resHdl;
  RefIndex numRefs;
  int16_t tableSize;
  RefTable *prt;
  int16_t err;

  //	Check id and file number and make sure compound

  /*
  DBG(DSRC_RES_ChkIdRef, {
    if (!ResCheckId(id))
      return (NULL);
  });
  */
  prd = RESDESC(id);
  fd = resFile[prd->filenum].fd;

  /*
  DBG(DSRC_RES_ChkIdRef, {
    if (fd < 0) {
      Warning(("ResReadRefTable: id $%x doesn't exist\n", id));
      return (NULL);
    }
  });
  */
  if ((ResFlags(id) & RDF_COMPOUND) == 0) {
    /*
    DBG(DSRC_RES_ChkIdRef,
        { Warning(("ResReadRefTable: id $%x is not compound\n", id)); });
    */
    return (NULL);
  }

  //	Seek to data, read numrefs, allocate table, read in offsets

  fseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
  fread(&numRefs, sizeof(RefIndex), 1, fd);
  prt = malloc(REFTABLESIZE(numRefs));
  prt->numRefs = numRefs;
  fread(&prt->offset[0], sizeof(int32_t) * (numRefs + 1), 1, fd);

  return (prt);
}

/*
//	---------------------------------------------------------
//
//	ResExtractRefTable() extracts a compound res's ref table.
//
//		id   = id of compound resource
//		prt  = ptr to ref table
//		size = size of ref table in bytes
//
//	Returns: 0 if ok, -1 if error

int32_t ResExtractRefTable(Id id, RefTable *prt, int32_t size)
{
        ResDesc *prd;
        int32_t fd;

//	Check id and file number and make sure compound

        DBG(DSRC_RES_ChkIdRef, {if (!ResCheckId(id)) return(-1);});
        prd = RESDESC(id);
        fd = resFile[prd->filenum].fd;
        DBG(DSRC_RES_ChkIdRef, {if (fd < 0) { \
                Warning(("ResExtractRefTable: id $%x doesn't exist\n", id)); \
                return(-1); \
                }});
        if ((ResFlags(id) & RDF_COMPOUND) == 0)
                {
                DBG(DSRC_RES_ChkIdRef, { \
                        Warning(("ResExtractRefTable: id $%x is not compound\n",
id)); \
                        });
                return(-1);
                }

//	Seek to data, read numrefs, check table size, read in offsets

        lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
        read(fd, &prt->numRefs, sizeof(RefIndex));
        if (REFTABLESIZE(prt->numRefs) > size)
                {
                Warning(("ResExtractRefTable: ref table too large for
buffer\n"));
                return(-1);
                }
        read(fd, &prt->offset[0], sizeof(int32_t) * (prt->numRefs + 1));

        return(0);
}

//	---------------------------------------------------------
//
// return number of refs, or -1 if error
//
//	---------------------------------------------------------
int32_t ResNumRefs(Id id)
{
        ResDesc *prd;

//	Check id and file number and make sure compound

        DBG(DSRC_RES_ChkIdRef, {if (!ResCheckId(id)) return(-1);});
        if ((ResFlags(id) & RDF_COMPOUND) == 0)
                {
                DBG(DSRC_RES_ChkIdRef, { \
                        Warning(("ResNumRefs: id $%x is not compound\n", id)); \
                        });
                return(-1);
                }
        prd = RESDESC(id);
   if (prd->ptr != NULL)
   {
      return ((RefTable*)prd->ptr)->numRefs;
   }
   else
   {
      int32_t fd = resFile[prd->filenum].fd;
      RefIndex result;
        DBG(DSRC_RES_ChkIdRef, {if (fd < 0) { \
                Warning(("ResNumRefs: id $%x doesn't exist\n", id)); \
                   return(-1); \
                   }});
        lseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
           read(fd, &result, sizeof(RefIndex));
      return result;
   }
}
*/

//	---------------------------------------------------------
//
//	RefExtract() extracts a ref item from a compound resource.
//
//		prt  = ptr to ref table
//		ref  = ref
//		buff = ptr to buffer (use RefSize() to compute needed buffer
// size)
//
//	Returns: ptr to supplied buffer, or NULL if problem
//	---------------------------------------------------------
//  For Mac version:  Use "ReadPartialResource" to get just the reference
//  specified.
//  ¥¥¥ For now, ignore LZW.

void *RefExtract(RefTable *prt, Ref ref, void *buff) {

  FILE *fd;
  int32_t index, offset, refsize;
  RefIndex numrefs;
  ResDesc *prd;

  //	Check id, get file number

  prd = RESDESC(REFID(ref));
  fd = resFile[prd->filenum].fd;
  index = REFINDEX(ref);

  // get reftable date from rt or by seeking.
  if (prt != NULL) {

    //#define RefSize(prt, index) (prt->offset[(index) + 1] -
    // prt->offset[index])
    refsize = RefSize(prt, index);
    numrefs = prt->numRefs;
    offset = prt->offset[index];
  } else {
    // seek into the file and find the stuff.
    fseek(fd, RES_OFFSET_DESC2REAL(prd->offset), SEEK_SET);
    fread(&numrefs, sizeof(RefIndex), 1, fd);
    fseek(fd, index * sizeof(int32_t), SEEK_CUR);
    fread(&offset, sizeof(int32_t), 1, fd);
    fread(&refsize, sizeof(int32_t), 1, fd);
    refsize -= offset;
    /*Warning(("Null reftable size = %d offset = %d numrefs = %d\n", refsize,
             offset, numrefs));*/
  }
  /*
  DBG(DSRC_RES_ChkIdRef, {
    if (!RefCheckRef(ref))
      return (NULL);
  });
  DBG(DSRC_RES_ChkIdRef, {
    if (index >= prt->numRefs) {
      Warning(("RefExtract: ref $%x index too large\n", ref));
      return (NULL);
    }
  });
  */
  prd = RESDESC(REFID(ref));
  fd = resFile[prd->filenum].fd;

  //	Add to cumulative stats

  CUMSTATS(REFID(ref), numExtracts);

  //	Seek to start of all data in compound resource

  fseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + REFTABLESIZE(numrefs),
        SEEK_SET);

  //	If LZW, extract with skipping, else seek & read

  if (ResFlags(REFID(ref)) & RDF_LZW) {
    LzwExpandFd2Buff(fd, buff,
                     offset - REFTABLESIZE(numrefs), // skip amt
                     refsize);
    // data amt
  } else {
    fseek(fd, offset - REFTABLESIZE(numrefs), SEEK_CUR);
    fread(buff, refsize, 1, fd);
  }

  return (buff);
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

//	Check id, get file number

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
      Warning(("Null reftable size = %d offset = %d numrefs = %d\n",refsize,offset,numrefs));
   }
        DBG(DSRC_RES_ChkIdRef, {if (!RefCheckRef(ref)) return(NULL);});
        DBG(DSRC_RES_ChkIdRef, {if (index >= numrefs) { \
                Warning(("RefExtract: ref $%x index too large\n", ref)); \
                return(NULL); \
                }});

//	Add to cumulative stats

        CUMSTATS(REFID(ref),numExtracts);

//	Seek to start of all data in compound resource

        lseek(fd, RES_OFFSET_DESC2REAL(prd->offset) + REFTABLESIZE(numrefs),
                SEEK_SET);


        lseek(fd, offset - REFTABLESIZE(numrefs), SEEK_CUR);
        return write(fd, buff, refsize);

}
*/

/*
//	---------------------------------------------------------
//		INTERNAL ROUTINES
//	---------------------------------------------------------
//
//	RefCheckRef() checks if ref valid.
//
//		ref = ref to be checked
//
//	Returns: true if ref ok, false if invalid & prints warning

uint8_t RefCheckRef(Ref ref)
{
        Id id;

        id = REFID(ref);
        if (!ResCheckId(id))
                return false;

        if ((ResFlags(id) & RDF_COMPOUND) == 0)
                {
                Warning(("RefCheckRef: id $%x is not a compound resource\n",
id));
                return false;
                }

        return true;
}
*/
