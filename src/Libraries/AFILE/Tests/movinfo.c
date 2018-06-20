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
//		MOVINFO		Dump info about movie
//		Rex E. Bradford

/*
 * $Source: r:/prj/lib/src/afile/RCS/movinfo.c $
 * $Revision: 1.5 $
 * $Author: rex $
 * $Date: 1994/10/24 12:53:52 $
 * $Log: movinfo.c $
 * Revision 1.5  1994/10/24  12:53:52  rex
 * Added chunk numbers
 *
 * Revision 1.4  1994/10/20  14:11:54  rex
 * Added chunk # to printout
 *
 * Revision 1.3  1994/10/18  20:15:52  rex
 * Reduced flags to 4 bits, adjusted tables accordingly
 *
 * Revision 1.2  1994/09/01  11:07:46  rex
 * Print out PALETTE chunk now
 *
 * Revision 1.1  1994/08/22  17:38:12  rex
 * Initial revision
 *
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fix.h"
#include "movie.h"

//	--------------------------------------------------------------
//		MAIN PROGRAM
//	--------------------------------------------------------------
int main(int argc, char *argv[]) {

  MovieHeader mh;
  static char *chunkNames[] = {"END  ", "VIDEO", "AUDIO", "TEXT ", "PAL  ", "TABLE", "?????", "?????"};
  static char *bmTypeNames[] = {
      "DEVICE", "MONO", "FLAT8", "FLAT24", "RSD8", "TLUC8", "SPAN", "GEN", "", "", "", "", "", "", "", "4X4",
  };
  static char *palNames[] = {
      "SET", "BLACK", "???", "???", "???", "???", "???", "???",
  };
  static char *tableNames[] = {
      "COLORSET", "HUFFTAB", "???", "???", "???", "???", "???", "???",
      "???",      "???",     "???", "???", "???", "???", "???", "???",
  };

  FILE *fpi;
  int32_t length;
  MovieChunk *pmc, *pmcBase;
  char *infile;

  if (argc < 2) {
    printf("Usage:  movinfo file.mov\n");
    exit(1);
  }
  infile = argv[1];

  //	Open input file

  fpi = fopen(infile, "rb");
  if (fpi == NULL) {
    printf("Can't open: %s\n", infile);
    exit(1);
  }

  //	Get movie header, check for valid movie file

  fread(&mh, sizeof(mh), 1, fpi);
  if (mh.magicId != MOVI_MAGIC_ID) {
    printf("%s not a valid .mov file!\n", infile);
    exit(1);
  }

  //	Dump movie header
  printf("Movie header:\n");
  printf("   num chunks:     %d\n", mh.numChunks);
  printf("   size chunks:    %dK\n", mh.sizeChunks >> 10);
  printf("   size data:      %d\n", mh.sizeData);
  printf("   total time:     %.04f\n", fix_float(mh.totalTime));
  printf("   frame rate:     %f\n", fix_float(mh.frameRate));
  printf("   frame size:     %d x %d\n", mh.frameWidth, mh.frameHeight);
  printf("   Video bits/pix: %d\n", mh.gfxNumBits);
  printf("   Palette:        %s\n", mh.isPalette ? "YES" : "NO");
  printf("   Audio channels: %d\n", mh.audioNumChans);
  printf("   Audio sampsize: %d\n", mh.audioSampleSize);
  printf("   Audio rate:     %d\n", fix_int(mh.audioSampleRate));

  //	If dumping chunks, do them

  pmc = (MovieChunk *)malloc(mh.sizeChunks);
  fread(pmc, mh.sizeChunks, 1, fpi);
  // That stupid, but seems that best way to read bitfields

  pmcBase = pmc;
  while (true) {
    // Print info for each chunk type.
    if (pmc->chunkType != MOVIE_CHUNK_END) {
      uint32_t nxoff = (pmc + 1)->offset;
      length = nxoff - pmc->offset;

      printf("[%04u] %s  time: %08.4f offset: %08d ($%06x) len: %05d ", (uint32_t)(pmc - pmcBase),
             chunkNames[pmc->chunkType], fix_float(pmc->time), pmc->offset, pmc->offset, length);

      switch (pmc->chunkType) {
      case MOVIE_CHUNK_VIDEO:
        printf("%s\n", bmTypeNames[pmc->flags & MOVIE_FVIDEO_BMTMASK]);
        break;

      case MOVIE_CHUNK_PALETTE:
        printf("%s", palNames[pmc->flags & MOVIE_FPAL_EFFECTMASK]);
        if (pmc->flags & MOVIE_FPAL_CLEAR)
          printf(" [CLEAR]");
        printf("\n");
        break;

      case MOVIE_CHUNK_TABLE:
        printf("%s\n", tableNames[pmc->flags]);
        break;

      case MOVIE_CHUNK_AUDIO:
      case MOVIE_CHUNK_TEXT:
      default:
        printf("\n");
        break;
      }
    } else {
      printf("END:          time: %08.4f\n", fix_float(pmc->time));
    }

    if (pmc->chunkType == MOVIE_CHUNK_END)
      break;
    ++pmc;
  }

  //	Close file

  fclose(fpi);
  return (0);
}
