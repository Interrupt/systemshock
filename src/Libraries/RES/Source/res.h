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
//              Res.H           Resource Manager header file
//              Rex E. Bradford (REX)
/*
 * $Header: n:/project/lib/src/res/rcs/res.h 1.9 1994/06/16 11:56:34 rex Exp $
 * $Log: res.h $
 * Revision 1.9  1994/06/16  11:56:34  rex
 * Got rid of RDF_NODROP
 *
 * Revision 1.8  1994/05/26  13:54:27  rex
 * Added prototype ResInstallPager()
 *
 * Revision 1.7  1994/03/09  19:31:48  jak
 * Res\RefExtractInBlocks transfers a variable/length
 * block of data in each pass.  The user/defined function
 * returns the amount that should be passed in NEXT time.
 *
 * Revision 1.6  1994/02/17  11:25:02  rex
 * Massive overhaul, moved some private stuff out to res_.h
 *
 * Revision 1.5  1993/09/01  16:02:10  rex
 * Added prototype for ResExtractRefTable().
 *
 * Revision 1.4  1993/05/13  10:38:44  rex
 * Added prototype for ResUnmake()
 *
 * Revision 1.3  1993/05/13  10:30:56  rex
 * Added Extract routines and macros
 *
 * Revision 1.2  1993/03/08  10:06:12  rex
 * Changed resource directory entry format (reduced from 12 to 10 bytes)
 *
 * Revision 1.1  1993/03/04  18:47:58  rex
 * Initial revision
 *
 * Revision 1.6  1993/03/02  18:42:21  rex
 * Major revision, new system
 *
 */

#ifndef __RES_H
#define __RES_H

//��� For now
//#define DBG_ON                1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h> // for FILE*
#include <stdlib.h>

#include "lg.h"

//#ifndef DATAPATH_H
//#include <datapath.h>
//#endif

#ifndef __RESTYPES_H
#include "restypes.h"
#endif

#pragma pack(push,2)

//      ---------------------------------------------------------
//              ID AND REF DEFINITIONS AND MACROS
//      ---------------------------------------------------------

//      Id's refer to resources, Ref's refer to items in compound resources

typedef uint16_t Id;       // ID of a resource
typedef uint32_t Ref;      // high word is ID, low word is index
typedef uint16_t RefIndex; // index part of ref

//      Here's how you get parts of a ref, or make a ref

#define REFID(ref) (uint16_t)((ref) >> 16u)                            // get id from ref
#define REFINDEX(ref) (uint16_t)((ref) & 0xFFFFu)                      // get index from ref
#define MKREF(id, index) ((((uint32_t)id) << 16u) | (uint16_t)(index)) // make ref

#define ID_NULL 0 // null resource id
#define ID_HEAD 1 // holds head ptr for LRU chain
#define ID_TAIL 2 // holds tail ptr for LRU chain
#define ID_MIN 3  // id's from 3 and up are valid

// Types of resource field within a resfile.
typedef enum {
    RFFT_PAD,      // not decoded, skip 'offset' bytes
    RFFT_UINT8,    // 8-bit integer
    RFFT_UINT16,   // 16-bit integer
    RFFT_UINT32,   // 32-bit integer
    RFFT_INTPTR,   // 32 bits on disc, 32 or 64 bits in memory.
    RFFT_RAW,      // raw data, copy 'offset' bytes or rest of resource if 0
    RFFT_END       // mark end of table
} ResFileFieldType;

// Describes the layout of a resource structure.
typedef struct {
    ResFileFieldType type;   // type of field
    size_t offset;           // offset in memory
} ResField;

typedef struct {
    size_t dsize;        // size of resource on disc
    size_t msize;        // size of resource in memory
    uint32_t flags;      // misc. info.
    ResField fields[];
} ResLayout;

// Indicates that multiple records exist within a resource, each of 'dsize'
// bytes, up to the resource size.
#define LAYOUT_FLAG_ARRAY            0x01
// Indicates that raw data (e.g. bitmap data) follows a header in the resource.
// The header is decoded according to the layout and the raw data is copied to
// immediately following it.
#define LAYOUT_FLAG_RAW_DATA_FOLLOWS 0x02

// User data for a resource decoding function.
typedef intptr_t UserDecodeData;
// Function to decode a resource loaded from disc. Takes raw data, size of raw
// data and user data. Returns decoded data. Default is to free decoded data
// using free() but the caller can supply a custom free function.
typedef void *(*ResDecodeFunc)(void*, size_t, UserDecodeData);
// Encoder function. Same signature as decoder; takes cooked data and returns
// raw data for file, violating the laws of thermodynamics but allowing portable
// saving. Encoded data is always freed using free().
typedef void *(*ResEncodeFunc)(void*, size_t, UserDecodeData);
// Function to free decoded data, if free() won't cut it.
typedef void (*ResFreeFunc)(void*);

// Decode a resource using a ResLayout. Prototyped as a decode function.
void *ResDecode(void *raw, size_t size, UserDecodeData layout);

// Describes the format of a resource for serialisation and deserialisation to
// and from disc file.
typedef struct {
    ResDecodeFunc decoder; // deserialise data from disc.
    ResEncodeFunc encoder; // serialise data to disc.
    UserDecodeData data;   // aux data, typically a pointer to a layout struct.
    ResFreeFunc freer;     // free cooked (only) data.
} ResourceFormat;

// An empty ResourceFormat struct means no translation is needed.
extern const ResourceFormat RawFormat;
#define FORMAT_RAW (&RawFormat)
// A null ResourceFormat means to preload the raw data and translate later.
#define FORMAT_PRELOAD ((ResourceFormat*)NULL)

//      ---------------------------------------------------------
//              ACCESS TO RESOURCES (ID'S)  (resacc.c)
//      ---------------------------------------------------------

void *ResLock(Id id, const ResourceFormat *format);   // lock resource & get ptr
void ResUnlock(Id id);                 // unlock resource
void *ResGet(Id id, const ResourceFormat *format);    // get ptr to resource (dangerous!)
void *ResExtract(Id id, void *buffer); // extract resource into buffer
void ResDrop(Id id);                   // drop resource from immediate use
void ResDelete(Id id);                 // delete resource forever

//      ------------------------------------------------------------
//              ACCESS TO ITEMS IN COMPOUND RESOURCES (REF'S)  (refacc.c)
//      ------------------------------------------------------------

// Each compound resource starts with a Ref Table. When loaded into memory, a
// ref table entry has a size, an offset into the raw resource (if loaded) and a
// pointer to the decoded ref (freed when the compound resource is unloaded).
typedef struct {
    uint32_t size;
    uint32_t offset;
    void *decoded_data;
} RefTableEntry;

typedef struct {
    RefIndex numRefs;  // # items in compound resource
    void *raw_data;    // resource data if loaded.
    RefTableEntry entries[]; // numRefs table entries
} RefTable;

// Decode raw ref table from file and return a RefTable.
void *ResDecodeRefTable(void *raw, size_t size, UserDecodeData);

void *RefLock(Ref ref, ResDecodeFunc decoder, UserDecodeData data, ResFreeFunc freer);                      // lock compound res, get ptr to item
#define RefLockRaw(ref) RefLock(ref, NULL, 0, NULL)
#define RefUnlock(ref) ResUnlock(REFID(ref)) // unlock compound res item
void *RefGet(Ref ref, ResDecodeFunc decoder, UserDecodeData data, ResFreeFunc freer);                       // get ptr to item in comp. res (dangerous!)
#define RefGetRaw(ref) RefGet(ref, NULL, 0, NULL)

RefTable *ResReadRefTable(Id id);        // alloc & read ref table
void ResFreeRefTable(void *ptr);         // free ref table
int32_t ResExtractRefTable(Id id, RefTable *prt,
                           int32_t size);             // extract reftable
// Get a ref table from a resource.
extern const ResourceFormat RefTableFormat;
#define FORMAT_REFTABLE (&RefTableFormat)

#define RefTableGet(ref) ((RefTable*)ResGet(ref, FORMAT_REFTABLE))
#define RefTableLock(ref) ((RefTable*)ResLock(ref, FORMAT_REFTABLE))
void *RefExtract(RefTable *prt, Ref ref, void *buff); // extract ref
// Extract a ref, decoding as we go
void *RefExtractDecoded(RefTable *prt, Ref ref, const ResLayout *layout, void *buff);

#define RefIndexValid(prt, index) ((index) < (prt)->numRefs)
#define RefSize(prt, index) (prt->entries[index].size)

// Size of a ref in memory once it is decoded, if the on-disc layout is different.
#define RefSizeDecoded(prt, index, layout) (RefSize(prt,index) + (layout)->msize - (layout)->dsize)

// returns the number of refs in a resource, extracting if necessary.
int32_t ResNumRefs(Id id);

#define REFTABLESIZE(numrefs) (offsetof(RefTable, entries) + ((numrefs) * sizeof(RefTableEntry)))

/*
//      -----------------------------------------------------------
//              BLOCK-AT-A-TIME ACCESS TO RESOURCES  (resexblk.c)
//      -----------------------------------------------------------

void ResExtractInBlocks(Id id, void *buff, int32_t blockSize,
        int32_t (*f_ProcBlock)(void *buff, int32_t numBytes, int32_t iblock));
void RefExtractInBlocks(RefTable *prt, Ref ref, void *buff, int32_t blockSize,
        int32_t (*f_ProcBlock)(void *buff, int32_t numBytes, int32_t iblock));

#define REBF_FIRST 0x01         // set for 1st block passed to f_ProcBlock
#define REBF_LAST  0x02         // set for last block (may also be first!)
*/

//      -----------------------------------------------------------
//              IN-MEMORY RESOURCE DESCRIPTORS, AND INFORMATION ROUTINES
//      -----------------------------------------------------------
//  For Mac version, keep a handle (rather than ptr).  Most ResDesc info not
//  needed because the Mac Resource Mgr takes care of it.

//      Each resource id gets one of these resource descriptors

/*typedef struct
{
        //Handle        hdl;                    // Mac resource handle.
NULL if not in memory (on disk) int32_t filenum;                // Mac
resource file number uint8_t    lock;                   // lock count uint8_t
flags;                  // misc flags (RDF_XXX, see below) uint8_t      type;
// resource type (RTYPE_XXX, see restypes.h) uint32_t   offset; uint32_t
size;

        void*   ptr;
        Id      next;
        Id      prev;
} ResDesc;*/

// It seems a little wasteful to keep the raw and decoded data alongside each
// other, but System Shock resources are not memory-hungry by modern standards
// and it allows code to preload resources without necessarily knowing how to
// decode them.
typedef struct {
    void *ptr;             // ptr to resource in memory, or NULL if on disk
    uint32_t lock : 8;     // lock count
    uint32_t size : 24;    // size of resource in bytes (1 Mb max)
    uint32_t filenum : 5;  // file number 0-31
    uint32_t offset : 27;  // offset in file
    Id next;               // next resource in LRU order
    Id prev;               // previous resource in LRU order
    /*uint32_t flags;*/    // misc flags (RDF_XXX, see below)
    /*uint16_t type : 8;*/ // resource type (RTYPE_XXX, see restypes.h)
    void *decoded;         // decoded data, if in-memory format is different.
    ResFreeFunc free_func; // free func; if NULL decoded is freed using free().
} ResDesc;

typedef struct {
    uint16_t flags : 8; // misc flags (RDF_XXX, see below)
    uint16_t type : 8;  // resource type (RTYPE_XXX, see restypes.h)
} ResDesc2;

#define RESDESC(id) (&gResDesc[id])      // convert id to resource desc ptr
#define RESDESC_ID(prd) ((prd)-gResDesc) // convert resdesc ptr to id

#define RESDESC2(id) (&gResDesc2[id])      // convert id to rd2 ptr
#define RESDESC2_ID(prd) ((prd)-gResDesc2) // convert rd2 ptr to id

#define RDF_LZW 0x01        // if 1, LZW compressed
#define RDF_COMPOUND 0x02   // if 1, compound resource
#define RDF_RESERVED 0x04   // reserved
#define RDF_LOADONOPEN 0x08 // if 1, load block when open file

#define RES_MAXLOCK 255 // max locks on a resource

// ptr to big array of ResDesc's
extern ResDesc *gResDesc;
// ptr to array of ResDesc2 (shared buff with resdesc)
extern ResDesc2 *gResDesc2;

extern Id resDescMax; // max id in res desc

//      Information about resources
#define ResInUse(id) (gResDesc[id].offset)
#define ResPtr(id) (gResDesc[id].ptr)
#define ResSize(id) (gResDesc[id].size)
#define ResLocked(id) (gResDesc[id].lock)
#define ResFilenum(id) (gResDesc[id].filenum)
#define ResType(id) (gResDesc2[id].type)
#define ResFlags(id) (gResDesc2[id].flags)
#define ResCompressed(id) (gResDesc2[id].flags & RDF_LZW)
#define ResIsCompound(id) (gResDesc2[id].flags & RDF_COMPOUND)
//#define ResZipped(id) (gResDesc2[id].flags & RDF_PKZIP)

//#define MaxSizeRsrc(theResource) GetMaxResourceSize(theResource)

//      ------------------------------------------------------------
//              RESOURCE MANAGER GENERAL ROUTINES  (res.c)
//      ------------------------------------------------------------

void ResInit(); // init Res, allocate initial ResDesc[]
void ResTerm(); // term Res (done auto via atexit)

//      ------------------------------------------------------------
//              RESOURCE FILE ACCESS (resfile.c)
//      ------------------------------------------------------------

typedef enum {
    ROM_READ,       // open for reading only
    ROM_EDIT,       // open for editing (r/w) only
    ROM_EDITCREATE, // open for editing, create if not found
    ROM_CREATE      // open for creation (deletes existing)
} ResOpenMode;

void ResAddPath(char *path); // add search path for resfiles
int32_t ResOpenResFile(const char *fname, ResOpenMode mode, bool auxinfo);
void ResCloseFile(int32_t filenum); // close res file

#define ResOpenFile(fname) ResOpenResFile(fname, ROM_READ, FALSE)
#define ResEditFile(fname, creat) ResOpenResFile(fname, (creat) ? ROM_EDITCREATE : ROM_EDIT, TRUE)
#define ResCreateFile(fname) ResOpenResFile(fname, ROM_CREATE, TRUE)

#define MAX_RESFILENUM 31 // maximum file number

// extern Datapath gDatapath;   // res system's datapath (others may use)
/*
//      ---------------------------------------------------------
//              RESOURCE MEMORY MANAGMENT ROUTINES  (resmem.c)
//      ---------------------------------------------------------

void *ResMalloc(size_t size);
void *ResRealloc(void *p, size_t newsize);
void ResFree(void *p);
void *ResPage(int32_t size);

void ResInstallPager(void *f(int32_t size));

//      ---------------------------------------------------------
//              RESOURCE STATS - ACCESSIBLE AT ANY TIME
//      ---------------------------------------------------------

typedef struct {
        uint16_t numLoaded;                                     // # resources
loaded in ram uint16_t numLocked;                                       // #
resources locked int32_t totMemAlloc;                                   // total
memory alloted to resources } ResStat;

extern ResStat resStat;                         // stats computed if
proper DBG bit set
*/

//      ----------------------------------------------------------
//              PUBLIC INTERFACE FOR CREATORS OF RESOURCES
//      ----------------------------------------------------------

//      ----------------------------------------------------------
//              RESOURCE MAKING  (resmake.c)
//      ----------------------------------------------------------

// make resource from data block
void ResMake(Id id, void *ptr, int32_t size, uint8_t type, int32_t filenum, uint8_t flags);
// make empty compound resource
void ResMakeCompound(Id id, uint8_t type, int32_t filenum, uint8_t flags);
// add item to compound
void ResAddRef(Ref ref, void *pitem, int32_t itemSize);
// unmake a resource
void ResUnmake(Id id);

//      ----------------------------------------------------------
//              RESOURCE FILE LAYOUT
//      ----------------------------------------------------------

//      Resource-file disk format:  header, data, dir

typedef struct {
    char signature[16];   // "LG ResFile v2.0\n",
    char comment[96];     // user comment, terminated with '\z'
    uint8_t reserved[12]; // reserved for future use, must be 0
    int32_t dirOffset;    // file offset of directory
} ResFileHeader;          // total 128 bytes (why not?)

typedef struct {
    uint16_t numEntries; // # items referred to by directory
    int32_t dataOffset;  // file offset at which data resides
                         // directory entries follow immediately
                         // (numEntries of them)
} ResDirHeader;

typedef struct {
    Id id;               // resource id (if 0, entry is deleted)
    uint32_t size : 24;  // uncompressed size (size in ram)
    uint32_t flags : 8;  // resource flags (RDF_XXX)
    uint32_t csize : 24; // compressed size (size on disk)
                         // (this size is valid disk size even if not comp.)
    uint32_t type : 8;   // resource type
} ResDirEntry;

// Active resource file table

typedef struct {
    uint16_t flags;         // RFF_XXX
    ResFileHeader hdr;      // file header
    ResDirHeader *pdir;     // ptr to resource directory
    uint16_t numAllocDir;   // # dir entries allocated
    int32_t currDataOffset; // current data offset in file
} ResEditInfo;

typedef struct {
    FILE *fd;           // file descriptor (from open())
    ResEditInfo *pedit; // editing info, or NULL if read-only file
} ResFile;

#define RFF_NEEDSPACK 0x0001 // resfile has holes, needs packing
#define RFF_AUTOPACK 0x0002  // resfile auto-packs (default TRUE)

extern ResFile resFile[MAX_RESFILENUM + 1];

// Macros to get ptr to resfile's directory, & iterate across entries

#define RESFILE_HASDIR(filenum) (resFile[filenum].pedit)
#define RESFILE_DIRPTR(filenum) (resFile[filenum].pedit->pdir)
#define RESFILE_DIRENTRY(pdir, n) ((ResDirEntry *)((pdir) + 1) + (n))
#define RESFILE_FORALLINDIR(pdir, pde) \
    for (pde = RESFILE_DIRENTRY(pdir, 0); pde < RESFILE_DIRENTRY(pdir, pdir->numEntries); pde++)

extern char resFileSignature[16]; // magic header

//      --------------------------------------------------------
//              RESOURCE FILE BUILDING  (resbuild.c)
//      --------------------------------------------------------

void ResSetComment(int32_t filenum, char *comment); // set comment
int32_t ResWrite(Id id);                            // write resource to file
void ResKill(Id id);                                // delete resource & remove from file
int32_t ResPack(int32_t filenum);                   // remove empty entries

//#define ResAutoPackOn(filenum) (resFile[filenum].pedit->flags |= RFF_AUTOPACK)
//#define ResAutoPackOff(filenum) (resFile[filenum].pedit->flags &=
//~RFF_AUTOPACK) #define ResNeedsPacking(filenum) (resFile[filenum].pedit->flags
//& RFF_NEEDSPACK)
// DG: a case-insensitive fopen()-wrapper (see resfile.c)
extern FILE *fopen_caseless(const char *path, const char *mode);

#pragma pack(pop)

#endif
