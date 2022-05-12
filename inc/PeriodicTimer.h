#pragma once

#include <stdint.h>

#define WIDE_TIMER_COUNT 6
typedef void (*periodic_driver_func)(void);
int WideTimer_Init(periodic_driver_func task, uint32_t period, uint32_t priority);
void WideTimer_Stop(periodic_driver_func task);
