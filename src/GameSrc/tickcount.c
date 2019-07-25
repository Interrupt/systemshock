//
// Created by winterheart on 25.07.19.
//

#include "tickcount.h"

// number of ticks since system start (1 Tick is about 1/60 second)
int32_t TickCount(void) {
    return (int32_t)((SDL_GetTicks() * 100) / 357); // 280 per second;
}