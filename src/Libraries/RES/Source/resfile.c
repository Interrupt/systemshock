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
//		ResFile.C		Resource Manager file access
//		Rex E. Bradford (REX)
/*
 * $Header: r:/prj/lib/src/res/rcs/resfile.c 1.5 1994/11/30 20:40:43 xemu Exp $
 * $Log: resfile.c $
 * Revision 1.5  1994/11/30  20:40:43  xemu
 * cd spoofing support
 *
 * Revision 1.4  1994/09/22  10:48:32  rex
 * Modified access to resdesc flags and type, which have moved
 *
 * Revision 1.3  1994/08/07  20:17:31  xemu
 * generate a warning on resource collision
 *
 * Revision 1.2  1994/06/16  11:07:04  rex
 * Added item to tail of LRU when loadonopen
 *
 * Revision 1.1  1994/02/17  11:23:23  rex
 * Initial revision
 *
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "lg.h"
#include "res.h"
#include "res_.h"

//	Resource files start with this signature

char resFileSignature[16] = {'L', 'G', ' ', 'R', 'e', 's', ' ', 'F', 'i', 'l', 'e', ' ', 'v', '2', 13, 10};

//	The active resource file info table

ResFile resFile[MAX_RESFILENUM + 1];

//	Global datapath for res files, other data modules may piggyback

// Datapath gDatapath;

int32_t ResFindFreeFilenum();

void ResReadDirEntries(int32_t filenum, ResDirHeader *pDirHead);
void ResProcDirEntry(ResDirEntry *pDirEntry, int32_t filenum, int32_t dataOffset);

void ResReadEditInfo(ResFile *prf);
void ResReadDir(ResFile *prf, int32_t filenum);
void ResCreateEditInfo(ResFile *prf, int32_t filenum);
void ResCreateDir(ResFile *prf);
void ResWriteDir(int32_t filenum);
void ResWriteHeader(int32_t filenum);

//	---------------------------------------------------------
//
//	ResAddPath() adds a path to the resource manager's list.
//
//		path = name of directory to add
/*
void ResAddPath(char *path)
{
        DatapathAdd(&gDatapath, path);

        Spew(DSRC_RES_General, ("ResAddPath: added %s\n", path));
}
*/

//	---------------------------------------------------------
//
//	ResOpenResFile() opens for read/edit/create.
//
//		fname   = ptr to filename
//		mode    = ROM_XXX (see res.h)
//		auxinfo = if TRUE, allocate aux info, including directory
//				  (applies to mode 0, other modes automatically
// get  it)
//
//	Returns:
//
//		-1 = couldn't find free filenum
//		-2 = couldn't open, edit, or create file
//		-3 = invalid resource file
//		-4 = memory allocation failure

int32_t ResOpenResFile(const char *fname, ResOpenMode mode, bool auxinfo) {
    int32_t filenum;
    FILE *fd;
    ResFile *prf;
    ResFileHeader fileHead;
    ResDirHeader dirHead;
    // uint8_t cd_spoof = FALSE;

    //	Find free file number, else return -1

    filenum = ResFindFreeFilenum();
    if (filenum < 0) {
        WARN("%s: no free filenum for: %s", __FUNCTION__, fname);
        return (-1);
    }

    //	If any mode but create, open along datapath.  If can't open,
    //	return error except if mode 2 (edit/create), in which case
    //	drop thru to create case by faking mode 3.

    TRACE("%s: %s", __FUNCTION__, fname);

    if (mode != ROM_CREATE) {
        //		fd = DatapathFDOpen(&gDatapath, fname, openMode[mode]);

        if (mode == ROM_READ)
            fd = fopen_caseless(fname, "rb");
        else
            fd = fopen_caseless(fname, "rb+");

        if (fd != NULL) {
            fread(&fileHead, sizeof(ResFileHeader), 1, fd);
            if (strncmp(fileHead.signature, resFileSignature, sizeof(resFileSignature)) != 0) {
                fclose(fd);
                WARN("%s: %s is not valid resource file", __FUNCTION__, fname);
                return (-3);
            }
        } else {
            if (mode == ROM_EDITCREATE)
                mode = ROM_CREATE;
            else {
                WARN("%s: can't open file: %s", __FUNCTION__, fname);
                return (-2);
            }
        }
    }

    //	If create mode, or edit/create failed, try to open file for creation.

    if (mode == ROM_CREATE) {
        fd = fopen_caseless(fname, "wb");
        if (fd == NULL) {
            WARN("%s: Can't create file: %s", __FUNCTION__, fname);
            return (-2);
        }
    }

    //	If aux info, allocate space for it

    prf = &resFile[filenum];
    prf->pedit = NULL;
    if (mode || auxinfo) {
        prf->pedit = (ResEditInfo *)malloc(sizeof(ResEditInfo));
        if (prf->pedit == NULL) {
            // Warning(("ResOpenResFile: unable to allocate ResEditInfo\n"));
            fclose(fd);
            return (-4);
        }
    }

    //	Record resFile[] file descriptor
    prf->fd = fd;
    TRACE("%s: opening: %s at filenum %d",__FUNCTION__, fname, filenum);

    // Switch based on mode
    switch (mode) {
    // If open existing file, read directory into edit info & process, or
    // if no edit info then process piecemeal.
    case ROM_READ:
    case ROM_EDIT:
    case ROM_EDITCREATE:
        if (prf->pedit) {
            ResReadEditInfo(prf);
            ResReadDir(prf, filenum);

            // Bugfix for save games growing the entries until crashing
            ResPack(filenum);
        } else {
            fseek(fd, fileHead.dirOffset, SEEK_SET);
            fread(&dirHead, 1, sizeof(ResDirHeader), fd);
            ResReadDirEntries(filenum, &dirHead);
        }
        break;

    // If open for create, initialize header & dir
    case ROM_CREATE:
        ResCreateEditInfo(prf, filenum);
        ResCreateDir(prf);
        break;
    }

    // Return filenum
    return (filenum);
}

//	---------------------------------------------------------
//
//	ResCloseFile() closes an open resource file.
//
//		filenum = file number used when opening file

void ResCloseFile(int32_t filenum) {
    Id id;

    // Make sure file is open
    if (resFile[filenum].fd == NULL) {
        WARN("%s: filenum %d not in use", __FUNCTION__, filenum);
        return;
    }

    // If file being created, flush it
    TRACE("%s: closing %d", __FUNCTION__, filenum);
    if (resFile[filenum].pedit) {
        ResWriteDir(filenum);
        ResWriteHeader(filenum);
    }

    // Scan object list, delete any blocks associated with this file
    for (id = ID_MIN; id <= resDescMax; id++) {
        if (ResInUse(id) && (ResFilenum(id) == filenum))
            ResDelete(id);
    }

    // Free up memory
    if (resFile[filenum].pedit) {
        if (resFile[filenum].pedit->pdir)
            free(resFile[filenum].pedit->pdir);
        free(resFile[filenum].pedit);
    }

    // Close file
    fclose(resFile[filenum].fd);
    resFile[filenum].fd = NULL;
}

//	--------------------------------------------------------------
//		INTERNAL ROUTINES
//	---------------------------------------------------------
//
//	ResFindFreeFilenum() finds free file number

int32_t ResFindFreeFilenum() {
    int32_t filenum;

    for (filenum = 1; filenum <= MAX_RESFILENUM; filenum++) {
        if (resFile[filenum].fd == NULL)
            return (filenum);
    }
    return (-1);
}

//	----------------------------------------------------------
//
//	ResReadDirEntries() reads in entries in a directory.
//		(file seek should be set to 1st directory entry)
//
//		filenum  = file number
//		pDirHead = ptr to directory header
//    add_flags = additional flags to OR into RDF flags for all
//                resources in this file.

void ResReadDirEntries(int32_t filenum, ResDirHeader *pDirHead) {
#define NUM_DIRENTRY_BLOCK 64 // (12 bytes each)
    FILE *fd;
    int32_t entry;
    int32_t dataOffset;
    ResDirEntry *pDirEntry;
    ResDirEntry dirEntries[NUM_DIRENTRY_BLOCK];

    // Set up
    pDirEntry = &dirEntries[NUM_DIRENTRY_BLOCK]; // no dir entries read
    dataOffset = pDirHead->dataOffset;           // mark starting offset
    fd = resFile[filenum].fd;

    // Scan directory:
    for (entry = 0; entry < pDirHead->numEntries; entry++) {
        // If reached end of local directory buffer, refill it
        if (pDirEntry >= &dirEntries[NUM_DIRENTRY_BLOCK]) {
            // read(fd, dirEntries, sizeof(ResDirEntry) * NUM_DIRENTRY_BLOCK);
            fread(dirEntries, sizeof(ResDirEntry) * NUM_DIRENTRY_BLOCK, 1, fd);
            pDirEntry = &dirEntries[0];
        }

        // Process entry
        ResProcDirEntry(pDirEntry, filenum, dataOffset);

        // Advance file offset and get next
        dataOffset = RES_OFFSET_ALIGN(dataOffset + pDirEntry->csize);
        pDirEntry++;
    }
}

//	-----------------------------------------------------------
//
//	ResProcDirEntry() processes directory entry, sets res desc.
//
//		pDirEntry  = ptr to directory entry
//		filenum    = file number
//		dataOffset = offset in file where data lives
//    add_flags = additional flags to OR into RDF flags for all
//                resources in this file.

void ResProcDirEntry(ResDirEntry *pDirEntry, int32_t filenum, int32_t dataOffset) {
    ResDesc *prd;
    ResDesc2 *prd2;
    int32_t currOffset;

    // Grow table if need to
    ResExtendDesc(pDirEntry->id);

    //TRACE("id %x", pDirEntry->id);

    // If already a resource at this id, warning
    prd = RESDESC(pDirEntry->id);
    prd2 = RESDESC2(pDirEntry->id);
    if (prd->ptr) {
        WARN("%s, RESOURCE ID COLLISION AT ID %x!!", __FUNCTION__, pDirEntry->id);
        ResDelete(pDirEntry->id);
    }
    
    // Fill in resource descriptor
    prd->ptr = NULL;
    prd->fsize = pDirEntry->size;
    prd->msize = 0; // not decoded yet
    prd->filenum = filenum;
    prd->lock = 0;
    prd->offset = RES_OFFSET_REAL2DESC(dataOffset);
    prd2->flags = pDirEntry->flags;
    prd2->type = pDirEntry->type;
    prd->next = 0;
    prd->prev = 0;

    //TRACE("Found id: %x of type %x", pDirEntry->id, pDirEntry->type);

    // If loadonopen flag set, load resource

    if (pDirEntry->flags & RDF_LOADONOPEN) {
        currOffset = ftell(resFile[filenum].fd);
	// Preload raw data, subsequent Lock() or Get() calls will decode if
	// the caller so wishes.
        ResLoadResource(pDirEntry->id, FORMAT_RAW);
        ResAddToTail(prd);
        fseek(resFile[filenum].fd, currOffset, SEEK_SET);
    }
}

//	--------------------------------------------------------------
//
//	ResReadEditInfo() reads edit info from file.

void ResReadEditInfo(ResFile *prf) {
    ResEditInfo *pedit = prf->pedit;

    // Init flags to no autopack or anything else
    pedit->flags = 0;

    // Seek to start of file, read in header
    fseek(prf->fd, 0L, SEEK_SET);
    fread(&pedit->hdr, sizeof(pedit->hdr), 1, prf->fd);

    // Set no directory (yet, anyway)
    pedit->pdir = NULL;
    pedit->numAllocDir = 0;
    pedit->currDataOffset = 0L;
}

//	---------------------------------------------------------------
//
//	ResReadDir() reads directory for a file.

void ResReadDir(ResFile *prf, int32_t filenum) {
    ResEditInfo *pedit;
    ResFileHeader *phead;
    ResDirHeader *pdir;
    ResDirEntry *pDirEntry;
    ResDirHeader dirHead;

    // Read directory header

    pedit = prf->pedit;
    phead = &pedit->hdr;
    fseek(prf->fd, phead->dirOffset, SEEK_SET);
    fread(&dirHead, sizeof(ResDirHeader), 1, prf->fd);

    // Allocate space for directory, copy directory header into it

    pedit->numAllocDir = (dirHead.numEntries + DEFAULT_RES_GROWDIRENTRIES) & ~(DEFAULT_RES_GROWDIRENTRIES - 1);
    pdir = pedit->pdir = malloc(sizeof(ResDirHeader) + (sizeof(ResDirEntry) * pedit->numAllocDir));
    *pdir = dirHead;

    // Read in directory into allocated space (past header)
    fread(RESFILE_DIRENTRY(pdir, 0), dirHead.numEntries * sizeof(ResDirEntry), 1, prf->fd);

    // Scan directory, setting resource descriptors & counting data bytes
    pedit->currDataOffset = pdir->dataOffset;

    RESFILE_FORALLINDIR(pdir, pDirEntry) {
        if (pDirEntry->id == 0)
            pedit->flags |= RFF_NEEDSPACK;
        else
            ResProcDirEntry(pDirEntry, filenum, pedit->currDataOffset);
        pedit->currDataOffset = RES_OFFSET_ALIGN(pedit->currDataOffset + pDirEntry->csize);
    }

    // Seek to current data location
    fseek(prf->fd, pedit->currDataOffset, SEEK_SET);
}

//	--------------------------------------------------------------
//
//	ResCreateEditInfo() creates new empty edit info.

void ResCreateEditInfo(ResFile *prf, int32_t filenum) {
    ResEditInfo *pedit = prf->pedit;

    pedit->flags = RFF_AUTOPACK;
    memcpy(pedit->hdr.signature, resFileSignature, sizeof(resFileSignature));
    ResSetComment(filenum, "");
    memset(pedit->hdr.reserved, 0, sizeof(pedit->hdr.reserved));
}

//	--------------------------------------------------------------
//
//	ResCreateDir() creates empty dir.

void ResCreateDir(ResFile *prf) {
    ResEditInfo *pedit = prf->pedit;

    pedit->hdr.dirOffset = 0;
    pedit->numAllocDir = DEFAULT_RES_GROWDIRENTRIES;
    pedit->pdir = malloc(sizeof(ResDirHeader) + (sizeof(ResDirEntry) * pedit->numAllocDir));
    pedit->pdir->numEntries = 0;
    pedit->currDataOffset = pedit->pdir->dataOffset = sizeof(ResFileHeader);
    fseek(prf->fd, pedit->currDataOffset, SEEK_SET);
}

//	-------------------------------------------------------------
//
//	ResWriteDir() writes directory to resource file.

void ResWriteDir(int32_t filenum) {
    ResFile *prf;

    if (resFile[filenum].pedit == NULL) {
        WARN("%s: file %d not open for writing", __FUNCTION__, filenum);
        return;
    }

    TRACE("%s: writing directory for filenum %d", __FUNCTION__, filenum);

    prf = &resFile[filenum];
    fseek(prf->fd, prf->pedit->currDataOffset, SEEK_SET);
    fwrite(prf->pedit->pdir, sizeof(ResDirHeader) + (prf->pedit->pdir->numEntries * sizeof(ResDirEntry)), 1, prf->fd);
}

//	--------------------------------------------------------
//
//	ResWriteHeader() writes header to resource file.

void ResWriteHeader(int32_t filenum) {
    ResFile *prf;

    if (resFile[filenum].pedit == NULL) {
        WARN("%s: file %d not open for writing", __FUNCTION__, filenum);
        return;
    }

    TRACE("%s: writing header for filenum %d", __FUNCTION__, filenum);

    prf = &resFile[filenum];
    prf->pedit->hdr.dirOffset = prf->pedit->currDataOffset;

    fseek(prf->fd, 0, SEEK_SET);
    fwrite(&prf->pedit->hdr, sizeof(ResFileHeader), 1, prf->fd);
}
