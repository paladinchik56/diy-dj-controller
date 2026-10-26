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

#include "analog.h"
#include "app_tasks.h"
#include "buttons.h"
#include "debug_uart.h"
#include "main.h"
#include "usb_midi.h"

#define USB_TASK_STACK_WORDS        256U
#define CONTROLS_TASK_STACK_WORDS   384U   /* prints the analog debug table */

#define USB_TASK_PRIORITY           ((UBaseType_t)osPriorityHigh)
#define CONTROLS_TASK_PRIORITY      ((UBaseType_t)osPriorityAboveNormal)

#define CONTROLS_PERIOD_MS          1U
#define ANALOG_SCAN_PERIOD_MS       4U     /* 250 full scans per second */

/* 1 = print raw ADC values every ANALOG_DEBUG_PERIOD_MS. Bench testing only:
 * each print blocks the controls task for ~10 ms at 115200 baud. */
#define ANALOG_DEBUG_PRINT          0
#define ANALOG_DEBUG_PERIOD_MS      500U

#define LED_BLINK_HALF_PERIOD_MS    500U

static void usb_task(void *argument)
{
    (void)argument;

    usb_midi_init();

    for (;;) {
        /* Sleeps until the USB interrupt queues an event, then handles it */
        usb_midi_task();
    }
}

/* Until USB-MIDI exists (step 7) changes are only printed */
static void on_analog_change(analog_id_t id, uint16_t value)
{
    debug_printf("%-10s -> %5u\n", analog_name(id), value);
}

/* Until USB-MIDI exists (step 7) button events are only printed */
static void on_button_event(const button_event_t *event)
{
    debug_printf("%-10s %s%s\n", buttons_name(event->id),
                 event->pressed ? "pressed" : "released",
                 event->shifted ? " (shift)" : "");
}

static void controls_task(void *argument)
{
    (void)argument;

    debug_printf("controls_task started, free heap = %u bytes\n", (unsigned)xPortGetFreeHeapSize());

    analog_init();
    buttons_init();

    /* vTaskDelayUntil() keeps a fixed period measured from this tick,
     * no matter how long the work inside the loop took. */
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t period_count = 0;   /* number of CONTROLS_PERIOD_MS periods so far */

    for (;;) {
        buttons_scan(on_button_event);

        if (period_count % (ANALOG_SCAN_PERIOD_MS / CONTROLS_PERIOD_MS) == 0U) {
            analog_scan();
            analog_process(on_analog_change);
        }

#if ANALOG_DEBUG_PRINT
        if (period_count % (ANALOG_DEBUG_PERIOD_MS / CONTROLS_PERIOD_MS) == 0U) {
            analog_debug_print();
        }
#endif

        period_count++;
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
