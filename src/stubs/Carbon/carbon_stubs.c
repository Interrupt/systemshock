#include "Carbon.h"

#include <SDL.h>
#include <stdlib.h>

#include "lg.h"

// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Toolbox/Toolbox-80.html
// number of ticks since system start (1 Tick is about 1/60 second)
int32_t TickCount(void)
{
	return (int32_t)((SDL_GetTicks() * 100) / 357);	// 280 per second;
}
