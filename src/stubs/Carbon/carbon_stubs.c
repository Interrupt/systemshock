#include "Carbon.h"

#include <SDL.h>
#include <stdlib.h>

#include "lg.h"

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

// yet another wrapper around malloc() and free() ?
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
	return (int32_t)((SDL_GetTicks() * 100) / 357);	// 280 per second;
}

void SetPort(/*GrafPtr*/ void* port)
{
	STUB_ONCE("I think this isn't needed anymore once all old drawing code is replaced with SDL");
}
