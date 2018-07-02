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
//		AMOV.C		Animfile handler for LG .mov files
//		Rex E. Bradford (REX)
//
/*
 * $Header: r:/prj/lib/src/afile/RCS/amov.c 1.6 1994/10/18 16:01:15 rex Exp $
 * $Log: amov.c $
 * Revision 1.6  1994/10/18  16:01:15  rex
 * Added processing of PALETTE chunk, so can return frame pals
 *
 * Revision 1.5  1994/10/04  10:34:50  rex
 * Fixed so can handle movies > 128 frames
 *
 * Revision 1.4  1994/10/03  18:04:33  rex
 * Added ability to read 4x4-compressed movies
 *
 * Revision 1.3  1994/09/29  10:31:00  rex
 * Added time arg to read/write frame
 *
 * Revision 1.2  1994/09/01  11:06:34  rex
 * Changed flag name
 *
 * Revision 1.1  1994/07/22  13:19:51  rex
 * Initial revision
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lg.h"
//#include <rsd.h>
#include "afile.h"
#include "compose.h"
#include "movie.h"
#include "rect.h"
//#include <decod4x4.h>
//#include <huff.h>

#include "movie.h"

//	Type-specific information

typedef struct {
    MovieHeader movieHdr;   // movie header
    MovieChunk *pmc;        // ptr to movie chunk array
    MovieChunk *pcurrChunk; // current chunk ptr
    FILE *fpTemp;           // temp file for writing
    uint8_t pal[768];       // space for palette
    uint8_t newPal;         // new pal flag
} AmovInfo;

//	Methods

int32_t AmovReadHeader(Afile *paf);
int32_t AmovReadFrame(Afile *paf, grs_bitmap *pbm, fix *ptime);
int32_t AmovReadFramePal(Afile *paf, Apalette *ppal);
int32_t AmovReadAudio(Afile *paf, void *paudio);
int32_t AmovReadReset(Afile *paf);
int32_t AmovReadClose(Afile *paf);

int32_t AmovWriteBegin(Afile *paf);
int32_t AmovWriteFrame(Afile *paf, grs_bitmap *pbm, int32_t bmlength, fix time);
int32_t AmovWriteClose(Afile *paf);

Amethods movMethods = {
    AmovReadHeader,   // Read header
    AmovReadFrame,    // Read frame
    AmovReadFramePal, // Read frame palette
    AmovReadAudio,    // f_ReadAudio
    AmovReadReset,
    AmovReadClose,
    AmovWriteBegin,
    NULL, // f_WriteAudio
    AmovWriteFrame,
    NULL, // f_WriteFramePal
    AmovWriteClose,
};

#define MAX_MOV_FRAMES 4096

#define MOV_TEMP_FILENAME "__movie_.tmp"

//	------------------------------------------------------
//		READER METHODS
//	------------------------------------------------------
//
//	AmovReadHeader() reads in movie file header & verifies.

int32_t AmovReadHeader(Afile *paf) {
    AmovInfo *pmi;
    MovieChunk *pchunk;

    //	Allocate type-specific info

    paf->pspec = malloc(sizeof(AmovInfo));
    pmi = (AmovInfo *)paf->pspec;

    // Read in movie header
    fread(&pmi->movieHdr, sizeof(pmi->movieHdr), 1, paf->fp);
    if (pmi->movieHdr.magicId != MOVI_MAGIC_ID) {
        free(paf->pspec);
        return (-1);
    }

    // Record header information
    paf->v.frameRate = pmi->movieHdr.frameRate;
    paf->v.width = pmi->movieHdr.frameWidth;
    paf->v.height = pmi->movieHdr.frameHeight;
    paf->v.numBits = pmi->movieHdr.gfxNumBits;
    if (pmi->movieHdr.isPalette) {
        paf->v.pal.index = 0;
        paf->v.pal.numcols = 256;
        memcpy(paf->v.pal.rgb, pmi->movieHdr.palette, 256 * 3);
    }

    // Audio information
    paf->a.numChans = pmi->movieHdr.audioNumChans;
    paf->a.sampleRate = pmi->movieHdr.audioSampleRate;
    paf->a.sampleSize = pmi->movieHdr.audioSampleSize;

    // Read in chunk offsets
    pmi->pmc = (MovieChunk *)malloc(pmi->movieHdr.sizeChunks);
    fread(pmi->pmc, pmi->movieHdr.sizeChunks, 1, paf->fp);

    // Compute # frames
    paf->v.numFrames = 0;
    for (pchunk = pmi->pmc; pchunk->chunkType != MOVIE_CHUNK_END; pchunk++) {
        if (pchunk->chunkType == MOVIE_CHUNK_VIDEO)
            paf->v.numFrames++;
        if (pchunk->chunkType == MOVIE_CHUNK_AUDIO)
            paf->a.numSamples++;
    }

    // No new palette
    pmi->newPal = false;

    // Current chunk is first one
    pmi->pcurrChunk = pmi->pmc;

    // Return

    return (0);
}

// DG: add some stubs so stuff builds

// for some reason precompiled.h is missing here?!
static void Draw4x4(const uint8_t *data, int16_t w, int16_t h) {
    // I have no idea what this really does..
    STUB_ONCE("TODO: Implement");
}
static void HuffExpandFlashTables(uint8_t *huffmanTable, uint32_t len, uint32_t *pl, int32_t xWTF) {
    // unsure about the types, especially of pl? and what it is as well..
    STUB_ONCE("TODO: Implement");
}
static void Draw4x4Reset(uint8_t *x, uint8_t *y) {
    // as you may have guessed, I have no idea about this one either.
    STUB_ONCE("TODO: Implement");
}
// DG end
//	----------------------------------------------------------
//
//	AmovReadFrame() reads the next frame.

int32_t AmovReadFrame(Afile *paf, grs_bitmap *pbm, fix *ptime) {
    static uint8_t *pColorSet;    // ptr to color set table (4x4 codec)
    static uint8_t *pHuffTabComp; // ptr to compressed huffman tab (4x4 codec)
    static uint8_t *pHuffTab;     // ptr to expanded huffman table (4x4 codec)

    AmovInfo *pmi;
    int32_t len;
    uint8_t *p;
    grs_canvas cv;

    pmi = (AmovInfo *)paf->pspec;

NEXT_CHUNK:

    switch (pmi->pcurrChunk->chunkType) {
    case MOVIE_CHUNK_END:
        return (-1);

    case MOVIE_CHUNK_VIDEO:
        pbm->type = pmi->pcurrChunk->flags & MOVIE_FVIDEO_BMTMASK;
        if (pbm->type == MOVIE_FVIDEO_BMF_4X4) {
            fseek(paf->fp, pmi->pcurrChunk->offset, SEEK_SET);
            len = MovieChunkLength(pmi->pcurrChunk);
            p = (uint8_t *)malloc(len);
            fread(p, len, 1, paf->fp);
            pbm->type = BMT_FLAT8;
            pbm->flags = BMF_TRANS;
            gr_make_canvas(pbm, &cv);
            gr_push_canvas(&cv);
            Draw4x4(p, paf->v.width, paf->v.height);
            gr_pop_canvas();
            free(p);
        } else {
            fseek(paf->fp, pmi->pcurrChunk->offset + sizeof(LGRect), SEEK_SET);
            len = MovieChunkLength(pmi->pcurrChunk) - sizeof(LGRect);
            fread(pbm->bits, len, 1, paf->fp);
        }
        *ptime = pmi->pcurrChunk->time;
        pmi->pcurrChunk++;
        return (len);

    case MOVIE_CHUNK_TABLE:
        fseek(paf->fp, pmi->pcurrChunk->offset, SEEK_SET);
        switch (pmi->pcurrChunk->flags) {
        case MOVIE_FTABLE_COLORSET:
            if (pColorSet)
                free(pColorSet);
            pColorSet = (uint8_t *)malloc(MovieChunkLength(pmi->pcurrChunk));
            fread(pColorSet, MovieChunkLength(pmi->pcurrChunk), 1, paf->fp);
            break;

        case MOVIE_FTABLE_HUFFTAB: {
            uint32_t len, *pl;
            pHuffTabComp = (uint8_t *)malloc(MovieChunkLength(pmi->pcurrChunk));
            fread(pHuffTabComp, MovieChunkLength(pmi->pcurrChunk), 1, paf->fp);
            pl = (uint32_t *)pHuffTabComp;
            len = *pl++;
            if (pHuffTab)
                free(pHuffTab);
            pHuffTab = (uint8_t *)malloc(len);
            HuffExpandFlashTables(pHuffTab, len, pl, 3);
            Draw4x4Reset(pColorSet, pHuffTab);
            free(pHuffTabComp);
        } break;
        }
        pmi->pcurrChunk++;
        goto NEXT_CHUNK;

    case MOVIE_CHUNK_PALETTE:
        if (pmi->pcurrChunk->flags == MOVIE_FPAL_SET) {
            fseek(paf->fp, pmi->pcurrChunk->offset, SEEK_SET);
            fread(pmi->pal, 768, 1, paf->fp);
            pmi->newPal = TRUE;
        }
        pmi->pcurrChunk++;
        goto NEXT_CHUNK;

    default:
        pmi->pcurrChunk++;
        goto NEXT_CHUNK;
    }
}

//	----------------------------------------------------------
//
//	AmovReadFramePal() reads pal for this frame just read.

int32_t AmovReadFramePal(Afile *paf, Apalette *ppal) {
    AmovInfo *pmi = (AmovInfo *)paf->pspec;

    ppal->index = 0;
    ppal->numcols = 0;

    if (pmi->newPal) {
        ppal->numcols = 256;
        memcpy(ppal->rgb, pmi->pal, 768);
        pmi->newPal = false;
        return false;
    }

    return false;
}

// Read audio data to buffer
int32_t AmovReadAudio(Afile *paf, void *paudio) {

    AmovInfo *pmi;
    uint32_t i = 0;
    void *p = (uint8_t *)malloc(MOVIE_DEFAULT_BLOCKLEN);

    pmi = (AmovInfo *)paf->pspec;
    while (pmi->pcurrChunk->chunkType != MOVIE_CHUNK_END) {
        // Got audio chunk
        if (pmi->pcurrChunk->chunkType == MOVIE_CHUNK_AUDIO) {
            TRACE("%s: got audio chunk in 0x%08x offset", __FUNCTION__, pmi->pcurrChunk->offset);
            fseek(paf->fp, pmi->pcurrChunk->offset, SEEK_SET);
            fread(p, MOVIE_DEFAULT_BLOCKLEN, 1, paf->fp);
            memcpy(paudio + i, p, MOVIE_DEFAULT_BLOCKLEN);
            i += MOVIE_DEFAULT_BLOCKLEN;
        }
        pmi->pcurrChunk++;
    }
    free(p);

    return 0;
}

//	----------------------------------------------------------
//
//	AmovReadReset() resets the movie for reading.

int32_t AmovReadReset(Afile *paf) {
    AmovInfo *pmi = (AmovInfo *)paf->pspec;

    pmi->pcurrChunk = pmi->pmc;

    return (0);
}

//	----------------------------------------------------------
//
//	AmovReadClose() does cleanup and closes file.

int32_t AmovReadClose(Afile *paf) {
    AmovInfo *pmi = (AmovInfo *)paf->pspec;

    free(pmi->pmc);
    free(pmi);
    fclose(paf->fp);

    return (0);
}

//	------------------------------------------------------
//		WRITER METHODS
//	------------------------------------------------------
//
//	AmovWriteBegin() starts up writer.

int32_t AmovWriteBegin(Afile *paf) {
    AmovInfo *pmi;

    //	Allocate type-specific info

    paf->pspec = calloc(1, sizeof(AmovInfo));
    pmi = paf->pspec;
    pmi->pmc = calloc(MAX_MOV_FRAMES, sizeof(MovieChunk));

    // Current chunk is first one
    pmi->pcurrChunk = pmi->pmc;

    // We want rsd!
    paf->writerWantsRsd = true;

    // Open temp file
    pmi->fpTemp = fopen(MOV_TEMP_FILENAME, "wb");
    if (pmi->fpTemp == NULL) {
        Warning(("AmovWriteBegin: can't open temp file\n"));
        return (-1);
    }

    // Return
    return (0);
}

//	------------------------------------------------------
//
//	AmovWriteFrame() writes out next frame.

int32_t AmovWriteFrame(Afile *paf, grs_bitmap *pbm, int32_t bmlength, fix time) {
    AmovInfo *pmi;
    LGRect area;

    pmi = paf->pspec;

    // Error-check

    if (paf->currFrame >= MAX_MOV_FRAMES) {
        WARN("%s: exceeded max # frames", __FUNCTION__);
        return (-1);
    }

    // Set current chunk
    pmi->pcurrChunk->time = time;
    pmi->pcurrChunk->chunkType = MOVIE_CHUNK_VIDEO;
    pmi->pcurrChunk->flags = pbm->type;
    pmi->pcurrChunk->offset = ftell(pmi->fpTemp);

    //	Write update area
    area.ul.x = 0;
    area.ul.y = 0;
    area.lr.x = pbm->w;
    area.lr.y = pbm->h;
    fwrite(&area, sizeof(area), 1, pmi->fpTemp);

    // Write bitmap
    fwrite(pbm->bits, bmlength, 1, pmi->fpTemp);

    // Update stuff
    pmi->pcurrChunk++;
    return (0);
}

//	-------------------------------------------------------
//
//	AmovWriteClose() closes output .mov

int32_t AmovWriteClose(Afile *paf) {
    AmovInfo *pmi;
    int32_t nc, numBlocks, numExtra;
    int32_t i;
    MovieChunk *pmc;
    uint8_t buff[2048];

    pmi = paf->pspec;

    // Set end chunk
    nc = pmi->pcurrChunk - pmi->pmc;
    if (nc == 0)
        pmi->pcurrChunk->time = 0;
    else if (nc == 1)
        pmi->pcurrChunk->time = (pmi->pcurrChunk - 1)->time * 2;
    else
        pmi->pcurrChunk->time =
            (pmi->pcurrChunk - 1)->time + ((pmi->pcurrChunk - 1)->time - (pmi->pcurrChunk - 2)->time);
    pmi->pcurrChunk->chunkType = MOVIE_CHUNK_END;
    pmi->pcurrChunk->flags = 0;
    pmi->pcurrChunk->offset = ftell(pmi->fpTemp);
    pmi->pcurrChunk++;

    // Set movie header and write out
    pmi->movieHdr.magicId = MOVI_MAGIC_ID;
    pmi->movieHdr.numChunks = pmi->pcurrChunk - pmi->pmc;
    pmi->movieHdr.sizeChunks = ((pmi->movieHdr.numChunks * sizeof(MovieChunk)) + 1023) & 0xFFFFFC00L;
    if ((pmi->movieHdr.sizeChunks & 0x400) == 0)
        pmi->movieHdr.sizeChunks += 0x0400; // 1K, 3K, 5K, etc.
    pmi->movieHdr.sizeData = ftell(pmi->fpTemp);
    pmi->movieHdr.totalTime = (pmi->pcurrChunk - 1)->time;
    pmi->movieHdr.frameRate = paf->v.frameRate;
    pmi->movieHdr.frameWidth = paf->v.width;
    pmi->movieHdr.frameHeight = paf->v.height;
    pmi->movieHdr.gfxNumBits = paf->v.numBits;
    pmi->movieHdr.isPalette = paf->v.pal.numcols != 0;

    // Skip audio for now

    if (pmi->movieHdr.isPalette)
        memcpy(&pmi->movieHdr.palette, &paf->v.pal.rgb[0], 768);

    // Adjust offsets
    for (pmc = pmi->pmc;; pmc++) {
        pmc->offset += (sizeof(MovieHeader) + pmi->movieHdr.sizeChunks);
        if (pmc->chunkType == MOVIE_CHUNK_END)
            break;
    }

    // Now close temp file, reopen, and copy to real file
    fclose(pmi->fpTemp);
    pmi->fpTemp = fopen(MOV_TEMP_FILENAME, "rb");
    if (pmi->fpTemp == NULL)
        WARN("%s: can't reopen temp file", __FUNCTION__);
    else {
        fwrite(&pmi->movieHdr, sizeof(MovieHeader), 1, paf->fp);
        fwrite(pmi->pmc, pmi->movieHdr.sizeChunks, 1, paf->fp);
        numBlocks = pmi->movieHdr.sizeData / sizeof(buff);
        numExtra = pmi->movieHdr.sizeData % sizeof(buff);
        for (i = 0; i < numBlocks; i++) {
            fread(buff, sizeof(buff), 1, pmi->fpTemp);
            fwrite(buff, sizeof(buff), 1, paf->fp);
        }
        if (numExtra) {
            fread(buff, numExtra, 1, pmi->fpTemp);
            fwrite(buff, numExtra, 1, paf->fp);
        }
    }

    // Free up stuff
    free(pmi->pmc);
    free(pmi);
    fclose(pmi->fpTemp);
    fclose(paf->fp);
    unlink(MOV_TEMP_FILENAME);
    return (0);
}
