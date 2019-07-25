#ifndef STUB_CARBON_H
#define STUB_CARBON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // memmove()

typedef unsigned char Boolean;

// http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/Toolbox/Toolbox-80.html
// number of ticks since system start (1 Tick is about 1/60 second)
int32_t TickCount(void);

#endif // STUB_CARBON_H
