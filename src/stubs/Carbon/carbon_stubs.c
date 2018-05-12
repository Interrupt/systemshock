#include "Carbon.h"

#include <SDL.h>

OSErr FSMakeFSSpec(short vRefNum, long dirID, /*ConstStr255Param*/ const char* fileName, FSSpec * spec)
{
	printf("STUB: FSMakeFSSpec(vRefNum = %hd, dirID = %ld, fileName = \"%s\", spec) - what does this do? is it needed?\n",
			vRefNum, dirID, fileName);

	return -37; // 	Bad filename or volume name - randomly chosen error code
}

Handle GetResource(ResType type, /*Integer*/ int id)
{
	uint8_t tStr[5];
	// TODO: could escape non-printable chars, but I guess that's not needed?
	tStr[0] = (uint8_t)(( type >> 24 ) & 255);
	tStr[1] = (uint8_t)(( type >> 16 ) & 255);
	tStr[2] = (uint8_t)(( type >> 8 ) & 255);
	tStr[3] = (uint8_t)(type & 255);
	tStr[4] = 0;

	printf("STUB: GetResource('%s', %d) - what is this about? is it needed?\n", tStr, id);
	return NULL;
}

//http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/Memory_Manager/memory_mgr_ref/function_group_1.html
Ptr NewPtr(Size byteCount)
{
	STUB_ONCE("probably just replace all calls to this with malloc()");
	Ptr ret = malloc(byteCount);
	return ret;
}

Ptr NewPtrClear(Size byteCount)
{
	STUB_ONCE("probably just replace all calls to this with calloc()");
	Ptr ret = calloc(byteCount);
	return ret;
}

// like free()
void DisposePtr(Ptr p)
{
	STUB_ONCE("probably just replace all calls to DisposePtr() with free()");
	free(p);
}

// yet another wrapper around malloc() and free() ...
// http://mirror.informatimago.com/next/developer.apple.com/documentation/Carbon/Reference/Memory_Manager/memory_mgr_ref/function_group_2.html#//apple_ref/c/func/NewHandle

Handle NewHandle(Size cnt)
{
	STUB_ONCE("Get rid of this!");
	return NULL;
}

void DisposeHandle(Handle h)
{
	STUB_ONCE("Get rid of this!");
}

// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Toolbox/Toolbox-80.html
// number of ticks since system start (1 Tick is about 1/60 second)
int32_t TickCount(void)
{
	Uint64 sdlTicks = SDL_GetTicks(); // in milliseconds, but we want 1/60 seconds
	sdlTicks *= 100;
	sdlTicks /= 1666;
	return (int32_t)sdlTicks;
}

void SetPort(/*GrafPtr*/ void* port)
{
	STUB_ONCE("I think this isn't needed anymore once all old drawing code is replaced with SDL");
}
