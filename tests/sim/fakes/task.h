#ifndef SIM_TASK_H
#define SIM_TASK_H

#include "FreeRTOS.h"

/* The simulation runs everything in one thread: nothing to lock */
#define taskENTER_CRITICAL()    do { } while (0)
#define taskEXIT_CRITICAL()     do { } while (0)

void vTaskDelay(TickType_t ticks);

#endif //SIM_TASK_H
