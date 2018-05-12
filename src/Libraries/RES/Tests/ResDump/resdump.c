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
//		Resdump.C		Resource File Dump Utility
//		Rex E. Bradford (REX)
//
//	This tool dumps information about resource files to the console.

/*
* $Header: n:/project/lib/src/res/rcs/resdump.c 1.5 1994/02/17 11:25:49 rex Exp
* $
* $Log: resdump.c $
 * Revision 1.5  1994/02/17  11:25:49  rex
 * Put brackets instead of quotes around include files
 *
 * Revision 1.4  1993/07/02  12:33:57  rex
 * Fixed so doesn't overwrite existing extension with .RES
 *
 * Revision 1.3  1993/04/15  11:17:53  rex
 * Fixed bug, dataOffset being incremented without taking pad bytes into accoun.
 *
 * Revision 1.2  1993/04/01  12:17:20  rex
 * Added "fulldump" hex byte dump option (-f)
 *
 * Revision 1.1  1993/03/04  18:48:35  rex
 * Initial revision
 *
 * Revision 1.1  1993/03/02  18:37:27  rex
 * Initial revision
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "res.h"
#include "res_.h"

//#define LISTALLFILES  1

int main(int argc, char *argv[]);
void DumpData(FILE *fd, int32_t offset, uint32_t size);

//	------------------------------------------
//		THE DUMP PROGRAM
//	------------------------------------------

int main(int argc, char *argv[]) {

  /*
  char *res_files[] = {
      "vidmail.res", "texture.res",  "objart.res",   "objart2.res",
      "objart3.res", "splash.res",   "gamepal.res",  "gamescr.res",
      "obj3d.res",   "citmat.res",   "cutspal.res",  "intro.res",
      "handart.res", "digifx.res",   "mfdart.res",   "mfdfrn.res",
      "mfdger.res",  "cybstrng.res", "frnstrng.res", "gerstrng.res",
      "death.res",   "start1.res",   "win1.res"};
  short fn;
  uchar foundsome;
  */

  char *fname;
  int i, j, filenum;
  FILE *fd;
  int32_t totsize, totcsize, compPct, numErased, sizeErased;
  ResFileHeader fileHead;
  ResDirHeader dirHead;
  ResDirEntry *pentry;
  uint8_t flagLzw, flagComp, flagLoad;
  RefIndex numEntries;
  int32_t dataOffset, *offset, sizeRef;
  bool doRefs, doFull;
  char buff[160];
  char tname[16];
  char blankPad[9] = {"        "};

  // uchar s1, s2;
  int32_t ofs, nextofs;

  //	Process args

  if (argc < 2) {
    printf("usage:  resdump resname.res [-f] [-r]\n");
    printf("        (-r = do references too)\n");
    printf("        (-f = full byte dump)\n");
    exit(1);
  }

  doRefs = doFull = false;
  for (j = 2; j < argc; j++) {
    if (strcmp(argv[j], "-r") == 0)
      doRefs = true;
    if (strcmp(argv[j], "-f") == 0)
      doFull = doRefs = true;
  }

#ifndef LISTALLFILES
  fname = argv[1];
#endif

  //	Init res system, open file

  ResInit();

#ifdef LISTALLFILES
  printf("\n");
  printf("  ID     TYPE     SIZE    CSIZE  PCT  LZW COMP LOAD\n");
  printf("-----  --------  ------  ------  ---  --- ---- ----\n");

  for (fn = 0; fn < 23; fn++) {
    strcpy(fname, res_files[fn]);
    foundsome = false;
#endif

    filenum = ResOpenFile(fname);
    if (filenum < 0) {
      printf("ResDump: can't open: %c\n", *fname);
      return -1;
    }

    //	Grab header and directory head and directory

    fd = resFile[filenum].fd;
    fseek(fd, 0L, SEEK_SET);
    fread(&fileHead, sizeof(ResFileHeader), 1, fd);
    fseek(fd, fileHead.dirOffset, SEEK_SET);
    fread(&dirHead, sizeof(ResDirHeader), 1, fd);

    pentry = (ResDirEntry *)malloc(dirHead.numEntries * sizeof(ResDirEntry));
    fread(pentry, dirHead.numEntries * sizeof(ResDirEntry), 1, fd);
    dataOffset = dirHead.dataOffset;

//	Print signature and comment

#ifndef LISTALLFILES
    printf("%s\n", fileHead.signature);
#endif

    //	Go thru directory, reporting

    totsize = totcsize = 0;
    numErased = sizeErased = 0;

    for (i = 0; i < dirHead.numEntries; i++) {
#ifndef LISTALLFILES
      if ((i & 15) == 0) {
        printf("\n");
        printf("Offset        ID     TYPE      SIZE    CSIZE  PCT  LZW COMP LOAD\n");
        printf("----------  ------  --------  ------  ------  ---  --- ---- ----\n");
      }
#endif

      compPct = ((pentry->size - pentry->csize) * 100) / pentry->size;

      flagLzw = (pentry->flags & RDF_LZW) ? '*' : ' ';
      flagComp = (pentry->flags & RDF_COMPOUND) ? '*' : ' ';
      flagLoad = (pentry->flags & RDF_LOADONOPEN) ? '*' : ' ';
      if (pentry->id) {
        sprintf(tname, "%s%s", resTypeNames[pentry->type],
                blankPad + strlen(resTypeNames[pentry->type]));
      } else {
        strcpy(tname, "ERASED  ");
        numErased++;
        sizeErased += pentry->csize;
      }

      sprintf(buff, "0x%08x    %04d  %s  %6d  %6d  %3d   %c    %c    %c",
              dataOffset, pentry->id, tname, pentry->size, pentry->csize,
              compPct, flagLzw, flagComp, flagLoad);

#ifdef LISTALLFILES
      // if ((pentry->flags & RDF_LZW) && (pentry->flags & RDF_COMPOUND))
      if (pentry->flags & RDF_LZW) {
        foundsome = true;
#endif
        puts(buff);
#ifdef LISTALLFILES
      }
#endif

      if (pentry->flags & RDF_COMPOUND) {
        if (doRefs) {
          fseek(fd, dataOffset, SEEK_SET);
          fread(&numEntries, sizeof(RefIndex), 1, fd);
          offset = malloc((numEntries + 1) * sizeof(long));
          fread(offset, sizeof(long) * (numEntries + 1), 1, fd);
          for (j = 0; j < numEntries; j++) {
            sizeRef = offset[j + 1] - offset[j];
            if ((long)sizeRef < 0) {
              sizeRef = offset[j + 1] - offset[j];
            }
            printf("\tIndex: $%4x  Offset: %6d  Size: %6d\n", j, offset[j], sizeRef);

            if (doFull) {
//              if (pentry->flags & RDF_LZW)
//                printf("\t(CAN'T DUMP BYTES - LZW COMPRESSED)\n");
//              else {
                DumpData(fd, dataOffset + offset[j], sizeRef);
//              }
            }
          }
          free(offset);
        }
      } else {
        if (doFull) {
//          if (pentry->flags & RDF_LZW)
//            printf("\t(CAN'T DUMP BYTES - LZW COMPRESSED)\n");
//          else {
            DumpData(fd, dataOffset, pentry->csize);
//          }
        }
      }

      totsize += pentry->size;
      totcsize += pentry->csize;
      dataOffset += pentry->csize + RES_OFFSET_PADBYTES(pentry->csize);
      pentry++;
    }

//	Report total statistics

#ifdef LISTALLFILES
    if (foundsome)
      //		printf("\nFound compressed compound resources in
      //'%s'.\n\n", fname);
      printf("\nFound compressed resources in '%s'.\n\n", fname);
#else
  printf("\nNumber of resource items: %d\n", dirHead.numEntries);
  if (numErased)
    printf("Num erased: %d, total size erased: %d\n", numErased, sizeErased);
  if (totsize) {
    printf("Total size: %d, total csize: %d, compression = %d%%\n", totsize,
           totcsize, ((totsize - totcsize) * 100) / totsize);
  } else {
    printf("Resource file empty!\n");
  }
#endif

    //	Close file & exit

    ResCloseFile(filenum);
  }

  /* This is a special case for movies

  void DumpData(FILE* fd, ulong offset, ulong size)
  {
          char 	*p;
          FILE	*outf;
          uchar 	buff[16384];
          size_t	amtRead;
          ulong	toRead;

          p = strchr(fname, '.');
          if (p == NULL)
          {
                  p = fname + strlen(fname);
                  *p = '.';
          }
          p++;
          strncpy(p, "mov", 3);

          outf = fopen(fname, "wb");
          if (outf == NULL)
          {
                  printf("Couldn't create .mov file\n");
                  return;
          }

          fseek(fd, offset, SEEK_SET);
          do
          {
                  toRead = sizeof(buff);
                  if (size < toRead)
                          toRead = size;
                  amtRead = fread(buff, 1, toRead, fd);
                  fwrite(buff, 1, amtRead, outf);
                  size -= amtRead;
          }
          while (amtRead == sizeof(buff));

          fclose(outf);
  }
  */

  void DumpData(FILE * fd, int32_t offset, uint32_t size) {
    int32_t oldpos;
    int32_t buffpos, i, j, l;
    uint8_t buff[512];
    uint8_t line[16];

    oldpos = ftell(fd);
    fseek(fd, offset, SEEK_SET);

    buffpos = sizeof(buff);
    l = 0;
    printf("\nHEX DUMP:\n");
    for (i = 0; i < size; i++) {
      if (buffpos == sizeof(buff)) {
        fread(buff, sizeof(buff), 1, fd);
        buffpos = 0;
      }
      line[l++] = buff[buffpos];
      printf("%02x ", buff[buffpos++]);
      if ((i & 15) == 15) {
        printf("  ");
//        for (j = 0; j < 16; j++)
//          printf("%c", line[j]);
        printf("\n");
        l = 0;
      }
    }
    if (i & 15) {
      for (j = l; j < 16; j++)
        printf("   ");
      printf("  ");
//      for (j = 0; j < l; j++)
//        printf("%c", line[j]);
      printf("\n");
    }
    printf("\n\n");

    fseek(fd, oldpos, SEEK_SET);
  }
