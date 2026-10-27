#ifndef SIM_FREERTOS_H
#define SIM_FREERTOS_H

/* Host simulation stand-in for FreeRTOS: one tick = 1 ms, single thread */

#include <stdint.h>

typedef uint32_t TickType_t;

#define pdMS_TO_TICKS(ms)   ((TickType_t)(ms))

#endif //SIM_FREERTOS_H
