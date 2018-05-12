#ifndef STUB_CARBON_H
#define STUB_CARBON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // memmove()

// pascal is used for some functions from Carbon/MacOS that use Pascal linkage or sth
// whatever, this define makes the compiler shut up for now :-P
#define pascal

#define nil NULL

typedef uint32_t ResType; // can hold four character codes ('ASDF')

typedef char* Ptr;
typedef Ptr* Handle;
typedef void* CGrafPtr; // whatever this is?

typedef Handle PixMapHandle; // whatever this is..
typedef Handle CTabHandle;   // ...

typedef unsigned char Boolean;

enum { noErr = 0 };

typedef int32_t Size; // long in MacOS, I guess that was a 32bit int?

typedef int32_t Fixed;

typedef int16_t OSErr;

typedef int32_t TimeValue;

typedef unsigned char Str255[256];

typedef struct FSSpec
{
	// TODO: what's in it? what's it good for?
} FSSpec;

typedef struct Point {
	short v;
	short h;
} Point;

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

OSErr FSMakeFSSpec(short vRefNum, long dirID, /*ConstStr255Param*/ const char* fileName, FSSpec * spec);

Handle GetResource(ResType type, /*Integer*/ int id);

static void ReleaseResource(Handle h) {}
static void HLock(Handle h) {}
static void HUnlock(Handle h) {}

extern Handle NewHandle(Size cnt);
extern void DisposeHandle(Handle h);
static void WriteResource (Handle theResource) {}

static void AddResource (Handle theData, ResType theType, short theID, /*ConstStr255Param*/ const char* name) {}

static void CloseResFile(short refNum) {}


static void numtostring(int num, char *str)
{
	sprintf(str, "%d", num);
}

static const char* c2pstr(const char* s)
{
	return s; // I hope no code (outside real Carbon) really relies on pascal strings..
}

// http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/Memory_Manager/memory_mgr_ref/function_group_1.html
extern Ptr NewPtr(Size byteCount);
extern Ptr NewPtrClear(Size byteCount);
extern void DisposePtr(Ptr p);

static OSErr MemError(void)
{
	// TODO: just eliminate all calls to this?
	return noErr;
}


// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Toolbox/Toolbox-80.html
// number of ticks since system start (1 Tick is about 1/60 second)
int32_t TickCount(void);

void SetPort(/*GrafPtr*/ void* port);

// http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/Memory_Manager/memory_mgr_ref/function_group_14.html#//apple_ref/c/func/BlockMoveData
static void BlockMoveData(const void* srcPtr, void* destPtr, Size byteCount)
{
	// docs say "If the value of byteCount is 0, BlockMove does nothing"
	// memmove() might assume (=> make the compilers optimizer believe)
	// that the pointers are != NULL.. thus the additional check here.
	if(byteCount > 0)
		memmove(destPtr, srcPtr, byteCount);
}
// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Memory/Memory-42.html
static void BlockMove(const void* srcPtr, void* destPtr, Size byteCount)
{
	if(byteCount > 0)
		memmove(destPtr, srcPtr, byteCount);
}

// the following functions are "implemented" in Stub.c, but don't turn up in any header
// so I added them here to make my compiler happy
// at least the Movie stuff is from QuickTime, so I assume the rest is Mac specific as well

void StopMovie(long);
void StartMovie(void * m);
Boolean IsMovieDone(long);

void HideCursor(void);
void ShowCursor(void);

typedef struct EventRecord EventRecord;
unsigned short GetOSEvent(short eventMask, EventRecord *theEvent);

void AdvanceProgress(void);
void EndProgressDlg(void);

//Boolean ShockAlertFilterProc(DialogPtr dlog, EventRecord *evt, short *itemHit) { return false; }


void MoviesTask(void *m, int n);

void DisposHandle(void *h);
void DisposeMovie(void *m);
void DisposCTable(void *c);

void BlitLargeAlign(unsigned char *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus);
void BlitLargeAlignSkip(unsigned char *draw_buffer, int dstRowBytes, void *dstPtr, long w, long h, long modulus);

#endif // STUB_CARBON_H
