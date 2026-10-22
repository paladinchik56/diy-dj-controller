#ifndef DJ_DEBUG_UART_H
#define DJ_DEBUG_UART_H

/*
 * Debug output over USART1 (PA9 TX / PA10 RX, 115200 8N1).
 * printf() is retargeted here, see debug_uart.c.
 */

/*
 * Creates the mutex used by debug_printf(). Call once before the scheduler starts.
 */
void debug_uart_init(void);

/*
 * Task-safe printf: holds a mutex for the whole format + transmit, so output
 * from different tasks never interleaves. Use this inside FreeRTOS tasks.
 * Plain printf() is fine before osKernelStart() (single thread).
 * Never call either of them from an interrupt handler.
 */
void debug_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/*
 * Writes a string straight to USART1: no mutex, no printf, no RTOS calls.
 * Only for fault paths (FreeRTOS hooks, error handlers) where the scheduler
 * state cannot be trusted. Works with interrupts disabled.
 */
void debug_uart_puts_unlocked(const char *s);

#endif //DJ_DEBUG_UART_H
