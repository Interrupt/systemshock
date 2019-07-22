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

#endif // STUB_CARBON_H
