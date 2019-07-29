//
// Created by winterheart on 25.07.19.
//

#ifndef TICKCOUNT_H_
#define TICKCOUNT_H_

#include <SDL.h>
#include <stdlib.h>

// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Toolbox/Toolbox-80.html
// number of ticks since system start (1 Tick is about 1/60 second)
int32_t TickCount(void);

#endif // TICKCOUNT_H_
