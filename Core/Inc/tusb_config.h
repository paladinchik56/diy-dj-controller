#ifndef DJ_TUSB_CONFIG_H
#define DJ_TUSB_CONFIG_H

/*
 * TinyUSB configuration (lib/tinyusb, included by tusb.h).
 * Device only, one USB-MIDI interface, running under FreeRTOS.
 */

/* MCU and OS */
#define CFG_TUSB_MCU                OPT_MCU_STM32F1     /* USB FS device peripheral, PA11/PA12 */
#define CFG_TUSB_OS                 OPT_OS_FREERTOS     /* tud_task() blocks on a queue */
#define CFG_TUSB_DEBUG              0

/* USB buffers must be 4-byte aligned for the packet memory copy routines */
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN          __attribute__((aligned(4)))

/* Device stack */
#define CFG_TUD_ENABLED             1
#define CFG_TUD_MAX_SPEED           OPT_MODE_FULL_SPEED /* F103 only does 12 Mbit/s */
#define CFG_TUD_ENDPOINT0_SIZE      64

/* Device classes: only MIDI */
#define CFG_TUD_CDC                 0
#define CFG_TUD_MSC                 0
#define CFG_TUD_HID                 0
#define CFG_TUD_MIDI                1
#define CFG_TUD_VENDOR              0

/* MIDI FIFO sizes in bytes: 64 = 16 USB-MIDI packets, one full-speed bulk packet */
#define CFG_TUD_MIDI_RX_BUFSIZE     64
#define CFG_TUD_MIDI_TX_BUFSIZE     64

#endif //DJ_TUSB_CONFIG_H
