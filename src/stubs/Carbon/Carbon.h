#ifndef STUB_CARBON_H
#define STUB_CARBON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // memmove()

typedef unsigned char Boolean;

typedef struct Rect {
  short               top;
  short               left;
  short               bottom;
  short               right;
} Rect;

// http://unix.superglobalmegacorp.com/xnu/newsrc/bsd/hfs/hfs_macos_defs.h.html
// https://developer.apple.com/documentation/coreservices/timerupp?language=objc
typedef struct TMTask TMTask;
typedef TMTask *TMTaskPtr;
typedef void (*TimerProcPtr)(TMTaskPtr tmTaskPtr);
typedef TimerProcPtr TimerUPP;
struct TMTask {
	//QElemPtr 						qLink;
	void*							qLink; // FIXME: ??
	short 							qType;
	TimerUPP 						tmAddr;
	long 							tmCount;
	long 							tmWakeUp;
	long 							tmReserved;
};

static void numtostring(int num, char *str)
{
	sprintf(str, "%d", num);
}

// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Toolbox/Toolbox-80.html
// number of ticks since system start (1 Tick is about 1/60 second)
int32_t TickCount(void);

// http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/Memory_Manager/memory_mgr_ref/function_group_14.html#//apple_ref/c/func/BlockMoveData
static void BlockMoveData(const void* srcPtr, void* destPtr, int32_t byteCount)
{
	// docs say "If the value of byteCount is 0, BlockMove does nothing"
	// memmove() might assume (=> make the compilers optimizer believe)
	// that the pointers are != NULL.. thus the additional check here.
	if(byteCount > 0)
		memmove(destPtr, srcPtr, byteCount);
}
// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Memory/Memory-42.html
static void BlockMove(const void* srcPtr, void* destPtr, int32_t byteCount)
{
	if(byteCount > 0)
		memmove(destPtr, srcPtr, byteCount);
}

#endif // STUB_CARBON_H
