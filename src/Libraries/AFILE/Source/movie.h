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
/*
 * $Source: r:/prj/lib/src/afile/RCS/movie.h $
 * $Revision: 1.16 $
 * $Author: dc $
 * $Date: 1994/12/01 04:17:50 $
 */

#ifndef __MOVIE_H
#define __MOVIE_H

#ifndef __FIX_H
#include "fix.h"
#endif
#ifndef __2D_H
#include "2d.h"
#endif
#ifndef __RES_H
#include "res.h"
#endif
#ifndef __CIRCBUFF_H
#include "circbuff.h"
#endif

#include "afile.h"

/*
#ifndef AIL_H
#include <ail.h>
#endif
*/
//	Movie file-format structures:
//
//	1. MovieHeader (1K) at head of file (check MOVI_MAGIC_ID)
//	2. MovieChunk[] array, padded to 1K/3K/5K (so array + hdr mult of 2K)
//	3. Actual chunks, as pointed at in MovieChunk[] array

//	Movie chunk format
#pragma pack(push,1)
// FIXME bitfield and little-endian hell of madness...
/*
typedef struct {
  uint32_t time : 24;     // fixed-point time since movie start
  uint32_t played : 1;    // has this chunk been clocked out?
  uint32_t flags : 4;     // chunkType-specific
  uint32_t chunkType : 3; // MOVIE_CHUNK_XXX
  uint32_t offset;        // int8_t offset to chunk start
} MovieChunk __attribute__ ((__packed__));;
*/
typedef struct {
    uint32_t time : 24;     // fixed-point time since movie start
    uint32_t chunkType : 3; // MOVIE_CHUNK_XXX
    uint32_t flags : 4;     // chunkType-specific
    uint32_t played : 1;    // has this chunk been clocked out?
    uint32_t offset;        // int8_t offset to chunk start
} MovieChunk;
#pragma pack(pop)

//	Movie chunk types

#define MOVIE_CHUNK_END 0
#define MOVIE_CHUNK_VIDEO 1
#define MOVIE_CHUNK_AUDIO 2
#define MOVIE_CHUNK_TEXT 3
#define MOVIE_CHUNK_PALETTE 4
#define MOVIE_CHUNK_TABLE 5

//	Movie chunk flags

#define MOVIE_FVIDEO_BMTMASK 0x0F // video chunk, 4 bits of flags is bmtype
#define MOVIE_FVIDEO_BMF_4X4 0x0F // 4x4 movie format

#define MOVIE_FPAL_EFFECTMASK 0x07 // pal chunk, 4 bits of flags is effect
#define MOVIE_FPAL_SET 0x00        // set palette from data
#define MOVIE_FPAL_BLACK 0x01      // set palette to black
#define MOVIE_FPAL_CLEAR 0x08      // if bit set, also clear screen

#define MOVIE_FTABLE_COLORSET 0 // table chunk, table is color set
#define MOVIE_FTABLE_HUFFTAB 1  // huffman table (compressed)

//	Movie header layout
#pragma pack(push,1)
typedef struct {
    uint32_t magicId;        // 'MOVI' (MOVI_MAGIC_ID)
    int32_t numChunks;       // number of chunks in movie
    int32_t sizeChunks;      // size in bytes of chunk array
    int32_t sizeData;        // size in bytes of chunk data
    fix totalTime;           // total playback time
    fix frameRate;           // frames/second, for info only
    int16_t frameWidth;      // frame width in pixels
    int16_t frameHeight;     // frame height in pixels
    int16_t gfxNumBits;      // 8, 15, 24
    int16_t isPalette;       // is palette present?
    int16_t audioNumChans;   // 0 = no audio, 1 = mono, 2 = stereo
    int16_t audioSampleSize; // 1 = 8-bit, 2 = 16-bit
    fix audioSampleRate;     // in Khz
    uint8_t reserved[216];   // so chunk is 1K in size
    uint8_t palette[768];    // palette
} MovieHeader;
#pragma pack(pop)

#ifndef SAMPRATE_11KHZ // also appear in voc.h
#define SAMPRATE_11KHZ fix_make(11127, 0)
#define SAMPRATE_22KHZ fix_make(22254, 0)
#endif

// FIXME: unportable, need change to FourCC
// Little endian, "MOVI"
#define MOVI_MAGIC_ID 0x49564F4D

//	Movie text chunk begins with a 0-terminated array of these:

typedef struct {
    uint32_t tag;    // 'XXXX'
    uint32_t offset; // offset to text string
} MovieTextItem;

#define MOVIE_TEXTITEM_MAKETAG(c1, c2, c3, c4) \
    ((((uint32_t)c4) << 24) | (((uint32_t)c3) << 16) | (((uint32_t)c2) << 8) | (c1))
#define MOVIE_TEXTITEM_TAG(pmti, index) ((pmti + (index))->tag)
#define MOVIE_TEXTITEM_PTR(pmti, index) ((char *)(pmti) + (pmti + (index))->offset)
#define MOVIE_TEXTITEM_EXISTS(pmti, index) MOVIE_TEXTITEM_TAG(pmti, index)

#define MOVIE_TEXTITEM_STDTAG MOVIE_TEXTITEM_MAKETAG('S', 'T', 'D', ' ')

//	Movie runtime structures

typedef struct {
    int16_t sizeBuffers; // size of each buffer
    uint8_t *pbuff[2];   // sound buffers (raw ptrs)
} MovieAudioBuffers;

typedef struct {
    CircBuff cb;            // circular data buffer
    int32_t blockLen;       // # bytes to read in each block
    int32_t ovfLen;         // # overflow bytes past circular buffer
    MovieChunk *pCurrChunk; // ptr to current chunk to use
    int32_t bytesLeft;      // bytes left to read
} MovieBuffInfo;

typedef struct {
    int32_t snd_in;
    int16_t nextBuff; // next buffer to load (0 or 1, -1 for none)
    int16_t smp_id;   // snd lib id of the current sample
} MovieAudioState;

typedef struct Movie_ {
    MovieHeader *pmh;                               // ptr to movie header (read from 1st bytes of movie)
    MovieChunk *pmc;                                // ptr to movie chunk array
    int32_t fd;                                     // file being read from
    int32_t fileOff;                                // offset in file to start of movie
    grs_canvas *pcanvas;                            // ptr to canvas being played into
    fix tStart;                                     // time movie started
    MovieBuffInfo bi;                               // movie buffering info
    MovieAudioState as;                             // current audio state for each channel
    uint8_t *pColorSet;                             // ptr to color set table (4x4 codec)
    int32_t lenColorSet;                            // length of color set table
    uint8_t *pHuffTab;                              // ptr to huffman table (4x4 codec)
    int32_t lenHuffTab;                             // length of huffman table
    void (*f_VideoCallback)(struct Movie_ *pmovie); // video callback for composing
    void (*f_TextCallback)(struct Movie_ *pmovie, MovieTextItem *pitem); // text chunk callback
    void *pTextCallbackInfo;                                             // info maintained by text callback
    uint8_t playing;                                                     // is movie playing?
    uint8_t processing;                                                  // is movie processing?
    uint8_t singleStep;                                                  // single step movie
    uint8_t clipCanvas;                                                  // clip to canvas?
} Movie;

//	Prototypes

Movie *MoviePrepare(int32_t fd, uint8_t *buff, int32_t buffLen, int32_t blockLen);
Movie *MoviePrepareRes(Id id, uint8_t *buff, int32_t buffLen, int32_t blockLen);
void MovieReadAhead(Movie *pmovie, int32_t numBlocks);
void MoviePlay(Movie *pmovie, grs_canvas *pcanvas);
void MovieUpdate(Movie *pmovie);
void MovieAdvance(Movie *pmovie);
void MovieRestart(Movie *pmovie);
void MovieKill(Movie *pmovie);

#define TXTCB_FLAG_CENTER_X 0x01
#define TXTCB_FLAG_CENTER_Y 0x02
#define TXTCB_FLAG_CENTERED TXTCB_FLAG_CENTER_X | TXTCB_FLAG_CENTER_Y

void MovieInstallStdTextCallback(Movie *pmovie, uint32_t lang, Id fontId, uint8_t color, uint8_t flags);

#define MovieChunkLength(pmc) (((pmc) + 1)->offset - (pmc)->offset)
#define MoviePlaying(pmovie) ((pmovie)->playing)
#define MovieSetSingleStep(pmovie, on) ((pmovie)->singleStep = (on))
#define MovieSetVideoCallback(pmovie, f) ((pmovie)->f_VideoCallback = (f))
#define MovieSetTextCallback(pmovie, f) ((pmovie)->f_TextCallback = (f))
#define MovieSetAudioBuffers(pmab) movieAudioBuffers = *(pmab)
#define MovieClearCanvas(pmovie)           \
    {                                      \
        gr_push_canvas((pmovie)->pcanvas); \
        gr_clear(0);                       \
        gr_pop_canvas();                   \
    }
#define MovieSetPal(pmovie, s, n) \
    if ((pmovie)->pmh->isPalette) \
    gr_set_pal(s, n, (pmovie)->pmh->palette)

extern MovieAudioBuffers movieAudioBuffers;

#define MOVIE_DEFAULT_BLOCKLEN 8192

// 4x4 cleanup routine (frees

void Draw4x4FreeResources();

// Custom functions
int32_t AfilePrepareRes(Id id, Afile *afile);
#endif
