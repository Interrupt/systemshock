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
//		Memall.C		Memory Allocation module
//		Rex E. Bradford (REX)
//
//		Memall provides a very simple mechanism for "installable memory
//		allocators".  The default allocator is malloc()/realloc()/free().
//		Clients may install and de-install their own handlers, in a pushdown
//		stack so that old allocators can be re-activated when the new allocator
//		is no longer needed.
//
//		To install a new allocator set, call:
//
//			MemPushAllocator(f_malloc, f_realloc, f_free);
//
//		To deinstall an allocator set, call:
//
//			MemPopAllocator();
//
//		A set of allocators is provided which features automatic checking for
//		allocation failure.  A convenience routine is provided to install
//		these:
//
//			MemCheckOn(uchar hard);	// will Error() or Warn() based on flag
//
//		To turn checking off, use:
//
//			MemCheckOff();
//
//		Also, a set of routines to malloc, realloc, and free conventional
//		(below 1 Mb) memory blocks is provided.  Since the regular memory
//		allocation system can usurp conventional memory, systems which need
//		conventional memory should allocate it early on in a program.

/*
* $Header: n:/project/lib/src/lg/rcs/memall.c 1.7 1993/08/11 18:43:55 rex Exp $
* $log$
*/

#include <string.h>
//#include <dos.h>
#include "memall.h"
#include <stdlib.h>
//#include "_lg.h"

//	Allocator set structure

typedef struct {
	void *(*func_malloc)(size_t size);				// allocator func
	void *(*func_realloc)(void *p, size_t size);	// realloc func
	void (*func_free)(void *p);						// de-allocator func
} MemAllocSet;

//	Allocator set stack

#define MAX_ALLOCATORS 4
static MemAllocSet memAllocStack[MAX_ALLOCATORS] = {
	malloc, realloc, free,
};
static int memIndexAllocStack = 0;

//	Current allocator ptrs

void *(*f_malloc)(size_t size) = malloc;
void *(*f_realloc)(void *p, size_t size) = realloc;
void (*f_free)(void *p) = free;

//	Miscellaneous

static uchar memHardCheck;		// if checking on, use Error or Warning?

#define INT_DPMI 0x31			// intr for Dos Protected Mode Interface

//	Internal prototypes

void *MallocChecked(size_t size);
void *ReallocChecked(void *p, size_t size);

//	--------------------------------------------------------------
//		SETTING, PUSHING, & POPPING ALLOCATOR SETS
//	--------------------------------------------------------------
//
//	MemSetAllocator() sets the current allocator set.
//
//		fm  = ptr to allocator function
//		ff  = ptr to free function
//		fr  = ptr to realloc function

void MemSetAllocator(void *(*fm)(size_t size),
	void *(fr)(void *p, size_t size), void (*ff)(void *p))
{
	MemAllocSet *pmas;

	pmas = &memAllocStack[memIndexAllocStack];
	f_malloc = pmas->func_malloc = fm;
	f_realloc = pmas->func_realloc = fr;
	f_free = pmas->func_free = ff;
}

//	--------------------------------------------------------------
//
//	MemPushAllocator() pushes old allocators, sets new one.
//
//		fm  = ptr to allocator function
//		ff  = ptr to free function
//		fr  = ptr to realloc function
//
//	Returns: 0 if successful, -1 if allocations stack full

int MemPushAllocator(void *(*fm)(size_t size),
	void *(fr)(void *p, size_t size), void (*ff)(void *p))
{
	if (memIndexAllocStack >= (MAX_ALLOCATORS - 1))
		return(-1);

	++memIndexAllocStack;
	MemSetAllocator(fm, fr, ff);
	return(0);
}

//	---------------------------------------------------------------
//
//	MemPopAllocator() pops most recent allocator.
//
//	Returns: 0 if successful, -1 if allocations stack underflow

int MemPopAllocator()
{
	MemAllocSet *pmas;

	if (memIndexAllocStack <= 0)
		return(-1);

	--memIndexAllocStack;
	pmas = &memAllocStack[memIndexAllocStack];
	f_malloc = pmas->func_malloc;
	f_realloc = pmas->func_realloc;
	f_free = pmas->func_free;
	return(0);
}

//	---------------------------------------------------------------
//		CALLOC - NONDEBUG VERSION
//	---------------------------------------------------------------
//
//	CallocNorm() allocates with Malloc(), then clears to 0.
//
//		size = # bytes to allocate and clear

void *CallocNorm(size_t size)
{
	void *p = (*f_malloc)(size);
	if (p)
		memset(p, 0, size);
	return(p);
}

//	---------------------------------------------------------------
//		SPEW VERSIONS
//	---------------------------------------------------------------
//
//	MallocSpew() does Malloc() and spews.

#ifdef DBG_ON

void *MallocSpew(size_t size, char *file, int line)
{
	void *p = (*f_malloc)(size);
	Spew(DSRC_LG_Memall, ("Malloc:  p: 0x%x  size: %d  (file: %s line: %d)\n",
		p, size, file, line));
	return(p);
}

//	---------------------------------------------------------------
//
//	ReallocSpew() does Realloc() and spews.

void *ReallocSpew(void *p, size_t size, char *file, int line)
{
	void *pnew = (*f_realloc)(p,size);
	Spew(DSRC_LG_Memall, ("Realloc: p: 0x%x  pold: 0x%x  size: %d  (file: %s line: %d)\n",
		pnew, p, size, file, line));
	return(pnew);
}

//	---------------------------------------------------------------
//
//	FreeSpew() does Free() and spews.

void FreeSpew(void *p, char *file, int line)
{
	(*f_free)(p);
	Spew(DSRC_LG_Memall, ("Free:    p: 0x%x  (file: %s line: %d)\n",
		p, file, line));
}

//	---------------------------------------------------------------
//
//	CallocSpew() does Calloc() and spews.

void *CallocSpew(size_t size, char *file, int line)
{
	void *p = (*f_malloc)(size);
	if (p)
		memset(p, 0, size);
	Spew(DSRC_LG_Memall, ("Calloc:  p: 0x%x  size: %d  (file: %s line: %d)\n",
		p, size, file, line));
	return(p);
}

#endif

//	---------------------------------------------------------------
//		CHECKED ALLOCATION
//	---------------------------------------------------------------
//
//	MemCheckOn() turns on memory checking.
//
//		hard = if TRUE, do hard error on alloc fail, else do warning

void MemCheckOn(uchar hard)
{
	MemPushAllocator(MallocChecked, ReallocChecked, f_free);
	memHardCheck = hard;
}

//	---------------------------------------------------------------
//
//	MemCheckOff() turns off memory checking.

void MemCheckOff()
{
	MemPopAllocator();
}

//	----------------------------------------------------------
//		CONVENTIONAL MEMORY ALLOCATION
//	----------------------------------------------------------
//
//	MallocConvMemBlock() allocates conventional memory.  It
//	returns a protected mode ptr as well as filling in a useful
//	structure, or returns NULL if unable to get the memory.
//
//		size = size of memory block in bytes
//		pcmb = ptr to ConvMemBlock structure (see res.h)
//
//	Returns: far ptr to block in low memory, or NULL

/*void *MallocConvMemBlock(ushort size, ConvMemBlock *pcmb)
{
	//union REGS regs;

//	Use DPMI to get the memory

	regs.x.eax = 0x0100;
	regs.x.ebx = (size + 15) >> 4;
	int386(INT_DPMI, &regs, &regs);
	if (regs.x.cflag)
		return(NULL);

//	Fill in our ConvMemBlock struct, return protected ptr

	pcmb->realSeg = regs.w.ax;
	pcmb->protSel = regs.w.dx;
//	pcmb->protPtr = MK_FP(pcmb->protSel, 0);	// this is the non-flat memory way
	pcmb->protPtr = (void *)(pcmb->realSeg << 4);
	return(pcmb->protPtr);
}*/

//	----------------------------------------------------------
//
//	ReallocConvMemBlock() resizes a conventional memory block.
//
//		pcmb    = ptr to ConvMemBlock structure (see res.h)
//		newsize = new size in bytes
//
//	Returns: far ptr to realloc'ed block

/*void *ReallocConvMemBlock(ConvMemBlock *pcmb, ushort newsize)
{
	//union REGS regs;
	long realAddr;

	regs.x.eax = 0x0102;
	regs.w.bx = (newsize + 15) >> 4;
	regs.w.dx = pcmb->protSel;
	int386(INT_DPMI, &regs, &regs);
	if (regs.x.cflag)
		return(NULL);

	regs.x.eax = 0x0006;
	regs.w.bx = pcmb->protSel;
	int386(INT_DPMI, &regs, &regs);
	realAddr = (((long) regs.w.cx) << 16) + regs.w.dx;
	pcmb->realSeg = realAddr >> 4;
//	pcmb->protPtr = MK_FP(pcmb->protSel, 0);	// this is the non-flat memory way
	pcmb->protPtr = (void *)(pcmb->realSeg << 4);

	malloc()

	return(pcmb->protPtr);
}*/

//	---------------------------------------------------------
//
//	FreeConvMemBlock() frees a conventional memory block.
//
//		pcmb = ptr to ConvMemBlock structure (see res.h)
//
//	Returns: 0 if successful, -1 if free failed

/*int FreeConvMemBlock(ConvMemBlock *pcmb)
{
	//union REGS regs;

	regs.x.eax = 0x0101;
	regs.w.dx = pcmb->protSel;
	int386(INT_DPMI, &regs, &regs);
	if (regs.x.cflag)
		return(-1);
	return(0);
}*/

//	---------------------------------------------------------------
//		INTERNAL ROUTINES
//	---------------------------------------------------------------
//
//	MallocChecked() calls the previously installed allocator, and
//		checks for NULL.  If underlying malloc failed, does hard error.
//
//		size = # bytes to allocate
//
//	Returns: ptr to memory block.

#define MallocPrev(size) (memIndexAllocStack >= 0 ? (*memAllocStack[memIndexAllocStack-1].func_malloc)(size) : NULL)

void *MallocChecked(size_t size)
{
	void *p;

	p = MallocPrev(size);
	/*if (p == NULL)
		{
		if (memHardCheck)
			Error(1, "MallocChecked: out of memory allocating %d bytes\n", size);
		else
			Warning(("MallocChecked: returning NULL (%d bytes requested)\n", size));
		}*/

	return(p);
}

//	----------------------------------------------------------------
//
//	ReallocChecked() calls the previously installed allocator, and
//		checks for NULL.  If underlying realloc failed, does hard error.
//
//		p    = ptr to existing block
//		size = new size
//
//	Returns: ptr to realloc'ed block.

#define ReallocPrev(p,size) (memIndexAllocStack >= 0 ? (*memAllocStack[memIndexAllocStack-1].func_realloc)(p,size) : NULL)

void *ReallocChecked(void *p, size_t size)
{
	void *pnew;

	pnew = ReallocPrev(p, size);
	/*if (pnew == NULL)
		{
		if (memHardCheck)
			Error(1, "ReallocChecked: out of memory reallocing %d bytes\n", size);
		else
			Warning(("ReallocChecked: returning NULL (%d bytes requested)\n", size));
		}*/

	return(pnew);
}
