/*
 * Application tasks.
 *
 * Priorities use the CMSIS-RTOS v2 scale, because CubeMX creates the "led"
 * task through osThreadNew() and cmsis_os2 passes osPriority values to
 * FreeRTOS unchanged (osPriorityLow = 8, Normal = 24, High = 40...).
 *   usb_task      osPriorityHigh         must answer the host quickly or USB stalls
 *   controls_task osPriorityAboveNormal  fixed 1 ms period for button/pot scanning
 *   led_task      set in CubeMX          cosmetic, runs when nothing else needs the CPU
 *
 * Stack sizes are in words (4 bytes on Cortex-M3), not bytes.
 * Tasks that print need more stack: newlib formatting uses a few hundred bytes.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"

#include "app_tasks.h"
#include "debug_uart.h"
#include "main.h"

#define USB_TASK_STACK_WORDS        256U
#define CONTROLS_TASK_STACK_WORDS   256U

#define USB_TASK_PRIORITY           ((UBaseType_t)osPriorityHigh)
#define CONTROLS_TASK_PRIORITY      ((UBaseType_t)osPriorityAboveNormal)

#define CONTROLS_PERIOD_MS          1U
#define LED_BLINK_HALF_PERIOD_MS    500U

static void usb_task(void *argument)
{
    (void)argument;

    for (;;) {
        /* TODO step 7: tud_task() from TinyUSB, it blocks until a USB event arrives */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void controls_task(void *argument)
{
    (void)argument;

    debug_printf("controls_task started, free heap = %u bytes\n", (unsigned)xPortGetFreeHeapSize());

    /* vTaskDelayUntil() keeps a fixed period measured from this tick,
     * no matter how long the work inside the loop took. */
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        /* TODO step 4-6: scan buttons and pots, send MIDI */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CONTROLS_PERIOD_MS));
    }
}

void led_task(void *argument)
{
    (void)argument;

    for (;;) {
        /* GPIO PC13: on-board Blue Pill LED, active-low. Blinking = scheduler alive. */
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_HALF_PERIOD_MS));
    }
}

void app_tasks_create(void)
{
    BaseType_t ok = pdPASS;

    ok &= xTaskCreate(usb_task,      "usb",      USB_TASK_STACK_WORDS,      NULL, USB_TASK_PRIORITY,      NULL);
    ok &= xTaskCreate(controls_task, "controls", CONTROLS_TASK_STACK_WORDS, NULL, CONTROLS_TASK_PRIORITY, NULL);

    /* pdPASS == 1, so a single failed create turns ok into 0 */
    if (ok != pdPASS) {
        Error_Handler();
    }
}
