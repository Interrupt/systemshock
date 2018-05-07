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
//		Restest.C	Resource system tester
//		Rex E. Bradford (REX)
/*
 * $Header: n:/project/lib/src/res/rcs/restest.c 1.7 1994/05/26 13:54:14 rex Exp
 * $
 * $log$
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "res.h"

//--------------------------------------
//  Prototypes
//--------------------------------------
int RestestHeapWalk();
void TestCreateFile(char *filename);
void TestDumpFile(char *filename);
void TestEditFile(char *filename);
void TestSpin(char *filename);
void TestRefExtract(char *filename);
void DumpBlock(char *p, short psize);

//----------------------------------------------------------------------------------
//  Main routine.
//----------------------------------------------------------------------------------
void main() {
  char ans[10];
  char c;
  char *reply = "test.res";

  ResInit();

  while (1) {
    printf("\n(C)reate, (D)ump, (E)dit, (R)ef Extract, (S)pin, (Q)uit : ");
    fgets(ans, sizeof(ans), stdin);
    c = toupper(ans[0]);
    switch (c) {
    case 'C':
      printf("Creating resource file %s\n", reply);
      TestCreateFile(reply);
      break;
    case 'D':
      printf("Dumping resource file %s\n", reply);
      TestDumpFile(reply);
      break;
    case 'E':
      printf("Editing resource file: %s\n", reply);
      TestEditFile(reply);
      break;
    case 'R':
      printf("Extracting resource file: %s\n", reply);
      TestRefExtract(reply);
      break;
    case 'S':
      printf("Spin a resource file: ");
      scanf("%s", reply);
      while (getchar() != '\n')
        ;
      if (reply)
        TestSpin(reply);
      break;
    case 'Q':
    case 27:
      ResTerm();
      exit(0);
    }
  };
}

//----------------------------------------------------------------------------------
//  Create a test file, add some resources to it.
//----------------------------------------------------------------------------------
void TestCreateFile(char *filename) {
  static uint8_t data1[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15};
  static uint8_t data2[] = {
      0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96,
      0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96,
      0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96,
      0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96,
      0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96,
      0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96, 0x99, 0x98, 0x97, 0x96,
  };
  static uint8_t data3[] = {
      0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x45, 0x45, 0x45,
      0x45, 0x45, 0x45, 0x45, 0x45, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
      0x44, 0x44, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
  };
  static uint8_t data4[] = {0x48, 0x48, 0x48, 0x48};
  // static uchar data5[] = {0x38,0x38};
  static uint8_t data5[] = {0x25, 0x25, 0x25, 0x26, 0x26, 0x26};

  short filenum;
  char * p;

  filenum = ResCreateFile(filename);
  printf("filenum = %d\n", filenum);

  ResSetComment(filenum, "This is a test\nresource file\n");

  p = malloc(sizeof(data1));
  memcpy(p, data1, sizeof(data1));
  ResMake(0x100, p, sizeof(data1), RTYPE_UNKNOWN, filenum, 0);

  p = malloc(sizeof(data2));
  memcpy(p, data2, sizeof(data2));
  ResMake(0x101, p, sizeof(data2), RTYPE_UNKNOWN, filenum, RDF_LZW);
  printf("resources added\n");

  ResMakeCompound(0x102, RTYPE_IMAGE, filenum, 0);
  ResAddRef(MKREF(0x102, 0), data3, sizeof(data3));
  ResAddRef(MKREF(0x102, 14), data4, sizeof(data4));
  ResAddRef(MKREF(0x102, 14), data5, sizeof(data5));
  ResAddRef(MKREF(0x102, 16), data4, sizeof(data4));
  printf("compound resource made\n");

  ResWrite(0x100);
  ResWrite(0x101);
  ResWrite(0x102);
  printf("resources written\n");

  ResCloseFile(filenum);
  printf("resource file closed\n");
  ResTerm();
}
/*
void TestDumpBlockDumper(void *buff, long numBytes, long iblock)
{
        char *p = buff;

        printf("block: %d (num: %d)   bytes: $%x $%x $%x $%x ...\n",
                iblock, numBytes, *p, *(p+1), *(p+2), *(p+3));
}
*/

//----------------------------------------------------------------------------------
//  Dump the bytes in each resource.
//----------------------------------------------------------------------------------
void TestDumpFile(char *filename) {
  int filenum, rs;
  Id id;
  char *p;
  ResDesc *prd;

  printf("opening file\n");
  filenum = ResOpenFile(filename);
  printf("filenum = %d\n", filenum);

  for (id = ID_MIN; id <= resDescMax; id++) {
    prd = RESDESC(id);
    if (prd->filenum == filenum) {
      ResLock(id);
      rs = ResSize(id);
      p = malloc(rs);
      ResExtract(id, p);
      ResUnlock(id);
      printf("Res 0x%x (size %d):\n", id, rs);
      DumpBlock(p, rs);
    }
  }

  /*
          printf("Reading resource in blocks\n");
          ResExtractInBlocks(0x100, buff, 4, TestDumpBlockDumper);
  */
  ResCloseFile(filenum);
  ResTerm();
}

//----------------------------------------------------------------------------------
//  Edit a res file (add and delete resources).
//----------------------------------------------------------------------------------
void TestEditFile(char *filename) {
  int filenum;
  char ans[10];
  int c;
  Id id;
  void *buff;

  filenum = ResEditFile(filename, true);
  if (filenum < 0) {
    printf("Error return: %d\n", filenum);
    return;
  }
  printf("File opened at filenum: %d\n", filenum);
  ResSetComment(filenum, "This file edited using ResEditFile");
  // ResAutoPackOff(filenum);

  while (1) {
    printf("(A)dd, (K)ill, (C)lose : ");
    fgets(ans, sizeof(ans), stdin);
    while (getchar() != '\n')
      ;
    switch (toupper(ans[0])) {
    case 'A':
      printf("Hit char for id : ");
      fgets(ans, sizeof(ans), stdin);
      while (getchar() != '\n')
        ;

      c = atoi(ans);
      id = 0x100 + c;
      buff = malloc(16);
      memset(buff, c, 16);
      ResMake(id, buff, sizeof(buff), 0, filenum, 0);
      c = ResWrite(id);
      printf("Wrote %d bytes\n", c);
      break;

    case 'K':
      printf("Hit char for id : ");
      fgets(ans, sizeof(ans), stdin);
      c = atoi(ans);
      id = 0x100 + c;
      ResKill(id);
      break;

    case 'C':
      ResCloseFile(filenum);
      return;
    }
  }
}

//----------------------------------------------------------------------------------
//  Who know what this does?  At least it doesn't crash.
//----------------------------------------------------------------------------------
void TestSpin(char *filename) {
  int filenum, i;
  Id id;
  uint8_t *p;

  printf("opening file\n");
  filenum = ResOpenFile(filename);
  printf("filenum = %d\n", filenum);

  for (i = 0; i < 1000; i++) {
    for (id = 0x100; id <= 0x101; id++) {
      p = (uint8_t *)ResLock(id);
      if (i == 999)
        printf("$%x: $%x $%x ... (size %d)\n", id, *p, *(p + 1), ResSize(id));
      ResUnlock(id);
    }
  }

  ResCloseFile(filenum);
}

//----------------------------------------------------------------------------------
//  Extract data from a resource using two different methods.
//----------------------------------------------------------------------------------
void TestRefExtract(char *filename) {
  int filenum;
  char ans[10];
  int c, rfs;
  Ref rid;
  char *p, cur;
  RefTable *rt;

  filenum = ResEditFile(filename, true);
  if (filenum < 0) {
    printf("Error return: %d\n", filenum);
    return;
  }
  printf("File opened at filenum: %d\n", filenum);

  printf("Extract method (1) or (2) or (L)ist table? ");
  fgets(ans, sizeof(ans), stdin);
  while (getchar() != '\n')
    ;
  switch (ans[0]) {
  case 'l':
  case 'L':
    rt = ResReadRefTable(0x102);
    for (c = 0; c <= rt->numRefs; c++)
      printf("Index:%d, Offset:%d\n", c, rt->offset[c]);
    break;

  case '1':
    printf("Ref index: ");
    fgets(ans, sizeof(ans), stdin);
    c = atoi(ans);
    rid = MKREF(0x102, c);
    p = RefLock(rid);
    // Copy into your own buffer here.
    RefUnlock(rid);
    break;

  case '2':
    printf("Ref index: ");
    fgets(ans, sizeof(ans), stdin);
    c = atoi(ans);
    rid = MKREF(0x102, c);

    rt = ResReadRefTable(REFID(rid));
    rfs = RefSize(rt, REFINDEX(rid));
    p = malloc(rfs);
    RefExtract(rt, rid, p);
    ResFreeRefTable(rt);

    DumpBlock(p, rfs);

    free(p);
    break;
  }
  ResCloseFile(filenum);
}

//----------------------------------------------------------------------------------
//  Dumps the contents of a pointer.
//----------------------------------------------------------------------------------
void DumpBlock(char *p, short psize) {
  short c = 1;
  char *cur = p;
  uint8_t ch;

  while ((cur - p) < psize) {
    ch = *cur;
    printf("0x%02x ", ch);
    c++;
    cur++;
    if (c > 8) {
      printf("\n");
      c = 1;
    } else if ((cur - p) >= psize)
      printf("\n");
  }
}
