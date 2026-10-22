/*
 * Debug output: retargets printf() to USART1.
 *
 * Peripheral: USART1, PA9 = TX, PA10 = RX, 115200 baud 8N1, no flow control
 * (configured in CubeMX, usart.c). Wire PA9 to RX of a USB-UART adapter and
 * GND to GND. Debug only, the panel itself talks to the host over USB-MIDI.
 */

#include <errno.h>
#include <unistd.h>

#include "usart.h"

/*
 * newlib calls _write() for everything printed to stdout/stderr.
 * syscalls.c has a weak version that sends one char at a time via
 * __io_putchar(); this strong definition replaces it at link time and
 * sends the whole buffer in a single blocking transfer.
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
