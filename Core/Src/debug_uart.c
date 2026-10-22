/*
 * Debug output: retargets printf() to USART1.
 *
 * Peripheral: USART1, PA9 = TX, PA10 = RX, 115200 baud 8N1, no flow control
 * (configured in CubeMX, usart.c). Wire PA9 to RX of a USB-UART adapter and
 * GND to GND. Debug only, the panel itself talks to the host over USB-MIDI.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "debug_uart.h"
#include "usart.h"

/*
 * newlib keeps one shared stdout state for all tasks (USE_NEWLIB_REENTRANT is
 * off to save ~1 KB of RAM per task), so the whole printf call, not just the
 * transmit, must be serialised.
 */
static SemaphoreHandle_t debug_uart_mutex;

void debug_uart_init(void)
{
    debug_uart_mutex = xSemaphoreCreateMutex();
}

void debug_printf(const char *fmt, ...)
{
    /* Lock only in task context with the scheduler running: before
     * osKernelStart() there is a single thread, and an ISR must never block. */
    const int use_lock = (debug_uart_mutex != NULL)
                      && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
                      && (__get_IPSR() == 0U);

    if (use_lock) {
        xSemaphoreTake(debug_uart_mutex, portMAX_DELAY);
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    /* Push out a line without '\n' while we still hold the lock */
    fflush(stdout);

    if (use_lock) {
        xSemaphoreGive(debug_uart_mutex);
    }
}

void debug_uart_puts_unlocked(const char *s)
{
    HAL_UART_Transmit(&huart1, (const uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

/*
 * newlib calls _write() for everything printed to stdout/stderr.
 * syscalls.c has a weak version that sends one char at a time via
 * __io_putchar(); this strong definition replaces it at link time and
 * sends the whole buffer in a single blocking transfer.
 * No locking here: debug_printf() already holds the mutex when it gets here.
 */
int _write(int file, char *ptr, int len)
{
    if (file != STDOUT_FILENO && file != STDERR_FILENO) {
        errno = EBADF;
        return -1;
    }

    if (HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY) != HAL_OK) {
        errno = EIO;
        return -1;
    }

    return len;
}
