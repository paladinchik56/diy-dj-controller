#ifndef DJ_APP_TASKS_H
#define DJ_APP_TASKS_H

/*
 * Creates the application FreeRTOS tasks (USB, controls).
 * Called from MX_FREERTOS_Init() in freertos.c, before osKernelStart().
 */
void app_tasks_create(void);

/*
 * Status LED task. Created by CubeMX in freertos.c (task "led", entry function
 * led_task, generated "As weak"); this strong definition replaces the weak stub.
 */
void led_task(void *argument);

#endif //DJ_APP_TASKS_H
