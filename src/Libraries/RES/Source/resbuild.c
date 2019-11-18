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
//    RESBUILD.C    Resource-file building routines
//    Rex E. Bradford (REX)
/*
 * $Header: n:/project/lib/src/res/rcs/resbuild.c 1.10 1994/06/16 11:06:30 rex
 * Exp $
 * $Log: resbuild.c $
 * Revision 1.10  1994/06/16  11:06:30  rex
 * Got rid of RDF_NODROP flag
 *
 * Revision 1.9  1994/02/17  11:25:32  rex
 * Moved some stuff out to resmake.c and resfile.c
 *
 */

#include <assert.h>
#include <string.h>
#if defined(_MSC_VER)
#include <windows.h>  // SetFilePointer / SetEndOfFile
#else
#include <unistd.h>   // ftruncate
#endif

#include "lg.h"
#include "lzw.h"
#include "res.h"
#include "res_.h"

#include <stdlib.h>

// make sure comment ends with one, so can type a file
#define CTRL_Z 26

bool ResEraseIfInFile(Id id);

//  Internal prototypes
static void ResCopyBytes(FILE *fd, int32_t writePos, int32_t readPos, int32_t size);

//  -------------------------------------------------------
//
//  ResSetComment() sets comment in res header.
//  -------------------------------------------------------
//  For Mac version:  Does nothing.  May go back later and add comment via
// the
// desktop database, maybe.

void ResSetComment(int32_t filenum, char *comment) {

    ResFileHeader *phead;
    if (resFile[filenum].pedit == NULL) {
        WARN("%s: file %d not open for writing", __FUNCTION__, filenum);
        return;
    }
    TRACE("%s: setting comment for filenum %d to: %s", __FUNCTION__, filenum, comment);


    phead = &resFile[filenum].pedit->hdr;
    memset(phead->comment, 0, sizeof(phead->comment));
    strncpy(phead->comment, comment, sizeof(phead->comment) - 2);
    phead->comment[strlen(phead->comment)] = CTRL_Z;
}

//  -------------------------------------------------------
//
//  ResWrite() writes a resource to an open resource file.
//  This routine assumes that the file position is already set to
//  the current data position.
//      Returns the total number of bytes written out, or -1 if there
//      was a writing error.
//
//    id = id to write
//  -------------------------------------------------------
#define EXTRA 250

int32_t ResWrite(Id id) {
    static uint8_t pad[] = {0, 0, 0, 0, 0, 0, 0, 0};
    ResDesc *prd;
    ResDesc2 *prd2;
    ResFile *prf;
    ResDirEntry *pDirEntry;
    uint8_t *p;
    size_t size, sizeTable;
    void *pcompbuff;
    int32_t compsize, padBytes;

    TRACE("%s: writing", __FUNCTION__);
    if (!ResCheckId(id))
        return -1;

    prd = RESDESC(id);
    prf = &resFile[prd->filenum];

    if (prf->pedit == NULL) {
        ERROR("%s: file %i not open for writing!", __FUNCTION__, prd->filenum);
        return -1;
    }
    //});

    // Check if item already in directory, if so erase it
    ResEraseIfInFile(id);

    // If directory full, grow it
    if (prf->pedit->pdir->numEntries == prf->pedit->numAllocDir) {
        TRACE("%s: growing directory of filenum %d", __FUNCTION__, prd->filenum);

        prf->pedit->numAllocDir += DEFAULT_RES_GROWDIRENTRIES;
        prf->pedit->pdir =
            realloc(prf->pedit->pdir, sizeof(ResDirHeader) + (sizeof(ResDirEntry) * prf->pedit->numAllocDir));
    }

    // Set resource's file offset
    prd->offset = RES_OFFSET_REAL2DESC(prf->pedit->currDataOffset);

    // See if it needs encoding.
    assert(prd->format != NULL);
    size = prd->msize;
    if (prd->format->encoder != NULL) {
	p = prd->format->encoder(prd->ptr, &size, prd->format->data);
    } else {
	p = prd->ptr;
    }
    prd->fsize = size;

    // Fill in directory entry
    pDirEntry = ((ResDirEntry *)(prf->pedit->pdir + 1)) + prf->pedit->pdir->numEntries;

    pDirEntry->id = id;
    prd2 = RESDESC2(id);
    pDirEntry->flags = prd2->flags;
    pDirEntry->type = prd2->type;
    pDirEntry->size = size;

    TRACE("%s: writing $%x\n", __FUNCTION__, id);

    // If compound, write out reftable without compression
    fseek(prf->fd, prf->pedit->currDataOffset, SEEK_SET);
    sizeTable = 0;

    // Body (post compound res header) if compound, else main pointer.
    uint8_t *body = p;
    // FIXME will need rework for compound refs if we reinstate, e.g. if we want
    // to integrate a resource/level editor.
    if (prd2->flags & RDF_COMPOUND) {
        sizeTable = REFTABLESIZE(((RefTable *)p)->numRefs);
        fwrite(p, sizeTable, 1, prf->fd);
        body = p + sizeTable;
        size -= sizeTable;
    }

    // If compression, try it (may not work out)
    if (pDirEntry->flags & RDF_LZW) {
        pcompbuff = malloc(size + EXTRA);
        compsize = LzwCompressBuff2Buff(p, size, pcompbuff, size);
        if (compsize < 0) {
            pDirEntry->flags &= ~RDF_LZW;
        } else {
            pDirEntry->csize = sizeTable + compsize;
            fwrite(pcompbuff, compsize, 1, prf->fd);
        }
        free(pcompbuff);
    }

    // If no compress (or failed to compress well), just write out
    if (!(pDirEntry->flags & RDF_LZW)) {
        pDirEntry->csize = size;
        fwrite(body, size, 1, prf->fd);
    }

    // Pad to align on data boundary
    padBytes = RES_OFFSET_PADBYTES(pDirEntry->csize);
    if (padBytes)
        fwrite(pad, padBytes, 1, prf->fd);

    // FIXME Error handling
    //  if (ftell(prf->fd) & 3)
    //    Warning(("ResWrite: misaligned writing!\n"));

    // If we encoded it, free the encode buffer.
    if (prd->format->encoder) {
	free(p);
    }
    // Advance dir num entries, current data offset
    prf->pedit->pdir->numEntries++;
    prf->pedit->currDataOffset = RES_OFFSET_ALIGN(prf->pedit->currDataOffset + pDirEntry->csize);

    return 0;
}

//  -------------------------------------------------------------
//
//  ResKill() not only deletes a resource from memory, it removes it
//  from the file too.
//  -------------------------------------------------------------
//  For Mac version:  Use Resource Manager to remove resource from file.  Have
//  to do
//  our own thing (instead of calling ResDelete()) because RmveResource turns
//  the resource handle into a normal handle.

void ResKill(Id id) {
    ResDesc *prd = RESDESC(id);

    if (prd->ptr) {
        if (prd->lock == 0)
            ResRemoveFromLRU(prd);
    }
    memset(prd, 0, sizeof(ResDesc));

    // Check for valid id
    if (!ResCheckId(id))
        return;
    TRACE("%s: killing $%x\n", __FUNCTION__, id);

    // Delete it
    ResDelete(id);

    // Make sure file is writeable
    prd = RESDESC(id);
    if (resFile[prd->filenum].pedit == NULL) {
        WARN("%s: file %d not open for writing", __FUNCTION__, prd->filenum);
        return;
    }

    // If so, erase it
    ResEraseIfInFile(id);
}

//  -------------------------------------------------------------
//
//  ResPack() removes holes from a resource file.
//
//    filenum = resource filenum (must already be open for
//                create/edit)
//
//  Returns: # bytes reclaimed

int32_t ResPack(int32_t filenum) {
    ResFile *prf;
    ResDirEntry *pDirEntry;
    int32_t numReclaimed, sizeReclaimed;
    int32_t dataRead, dataWrite;
    int32_t i;
    ResDirEntry *peWrite;

    // Check for errors
    prf = &resFile[filenum];
    if (prf->pedit == NULL) {
        ERROR("%s: filenum %d not open for editing", __FUNCTION__, filenum);
        return (0);
    }

    // Set up
    sizeReclaimed = numReclaimed = 0;
    dataRead = dataWrite = prf->pedit->pdir->dataOffset;

    // Scan thru directory, copying over all empty entries
    pDirEntry = (ResDirEntry *)(prf->pedit->pdir + 1);
    for (i = 0; i < prf->pedit->pdir->numEntries; i++) {
        if (pDirEntry->id == 0) {
            numReclaimed++;
            sizeReclaimed += pDirEntry->csize;
        } else {
            if (gResDesc[pDirEntry->id].offset > RES_OFFSET_PENDING)
                gResDesc[pDirEntry->id].offset = RES_OFFSET_REAL2DESC(dataWrite);
            if (dataRead != dataWrite)
                ResCopyBytes(prf->fd, dataWrite, dataRead, pDirEntry->csize);
            dataWrite = RES_OFFSET_ALIGN(dataWrite + pDirEntry->csize);
        }
        dataRead = RES_OFFSET_ALIGN(dataRead + pDirEntry->csize);
        pDirEntry++;
    }

    // Now pack directory itself
    pDirEntry = (ResDirEntry *)(prf->pedit->pdir + 1);
    peWrite = pDirEntry;
    for (i = 0; i < prf->pedit->pdir->numEntries; i++) {
        if (pDirEntry->id) {
            if (pDirEntry != peWrite)
                *peWrite = *pDirEntry;
            peWrite++;
        }
        pDirEntry++;
    }
    prf->pedit->pdir->numEntries -= numReclaimed;

    // Set new current data offset
    prf->pedit->currDataOffset = dataWrite;
    fseek(prf->fd, dataWrite, SEEK_SET);
    prf->pedit->flags &= ~RFF_NEEDSPACK;

    // Truncate file to just header & data (will be extended later when
    // write directory on closing)

    // FIXME Non-portable
#ifndef _MSC_VER
    ftruncate(fileno(prf->fd), dataWrite);
#else // So much for POSIX.
    SetFilePointer(fileno(prf->fd), dataWrite, NULL, FILE_BEGIN);
    SetEndOfFile(fileno(prf->fd));
#endif

    // Return # bytes reclaimed
    TRACE("%s: reclaimed %d bytes", __FUNCTION__, sizeReclaimed);

    return (sizeReclaimed);
}

#define SIZE_RESCOPY 32768

static void ResCopyBytes(FILE *fd, int32_t writePos, int32_t readPos, int32_t size) {
    int32_t sizeCopy;
    uint8_t *buff;

    buff = malloc(SIZE_RESCOPY);

    while (size > 0) {
        sizeCopy = lg_min(SIZE_RESCOPY, size);
        fseek(fd, readPos, SEEK_SET);
        fread(buff, sizeCopy, 1, fd);
        fseek(fd, writePos, SEEK_SET);
        fwrite(buff, sizeCopy, 1, fd);
        readPos += sizeCopy;
        writePos += sizeCopy;
        size -= sizeCopy;
    }

    free(buff);
}

//  --------------------------------------------------------
//    INTERNAL ROUTINES
//  --------------------------------------------------------
//
//  ResEraseIfInFile() erases a resource if it's in a file's directory.
//
//    id = id of item
//
//  Returns: TRUE if found & erased, FALSE otherwise

bool ResEraseIfInFile(Id id) {
    ResDesc *prd;
    ResFile *prf;
    ResDirEntry *pDirEntry;
    int32_t i;

    prd = RESDESC(id);
    prf = &resFile[prd->filenum];
    pDirEntry = (ResDirEntry *)(prf->pedit->pdir + 1);

    for (i = 0; i < prf->pedit->pdir->numEntries; i++) {
        if (id == pDirEntry->id) {
            TRACE("%s: $%x being erased\n", __FUNCTION__, id);
            pDirEntry->id = 0;
            prf->pedit->flags |= RFF_NEEDSPACK;
            if (prf->pedit->flags & RFF_AUTOPACK)
                ResPack(prd->filenum);
            return true;
        }
        pDirEntry++;
    }

    return false;
}
