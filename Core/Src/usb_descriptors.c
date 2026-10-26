/*
 * USB descriptors: how the controller introduces itself to the host.
 * TinyUSB asks for them through the tud_descriptor_*_cb() callbacks below
 * while the host enumerates the device.
 */

#include <string.h>

#include "tusb.h"
#include "main.h"

/*
 * 0x1209 is the pid.codes open-source vendor ID; 0x0001 is its "test PID",
 * allowed for private/hobby devices. Request an own PID from pid.codes
 * before distributing the device to other people.
 */
#define USB_VID     0x1209
#define USB_PID     0x0001
#define USB_BCD     0x0100      /* device release 1.00 */

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,     /* class is defined per interface */
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = USB_BCD,
    .iManufacturer      = 1,        /* indexes into string_descriptors[] */
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&desc_device;
}

/* A USB-MIDI function is two interfaces: Audio Control + MIDI Streaming */
enum {
    ITF_NUM_MIDI = 0,
    ITF_NUM_MIDI_STREAMING,
    ITF_NUM_TOTAL
};

#define EPNUM_MIDI_OUT      0x01    /* host -> device, bulk */
#define EPNUM_MIDI_IN       0x81    /* device -> host, bulk */
#define MIDI_EP_SIZE        64      /* max bulk packet at full speed */

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

static const uint8_t desc_configuration[] = {
    /* config number, interface count, string index, total length, attributes, max power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    /* interface number, string index, EP out, EP in, EP size */
    TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 0, EPNUM_MIDI_OUT, EPNUM_MIDI_IN, MIDI_EP_SIZE),
};

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;    /* only one configuration */
    return desc_configuration;
}

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

/* Product is the name DJ software and the OS show for the MIDI port */
static const char *const string_descriptors[] = {
    [STRID_MANUFACTURER] = "DIY",
    [STRID_PRODUCT]      = "DIY DJ Controller",
};

#define STRING_MAX_CHARS    32

/* USB strings are UTF-16LE; element 0 holds the descriptor length and type */
static uint16_t string_buffer[STRING_MAX_CHARS + 1];

/*
 * Serial number: the STM32 96-bit unique ID (factory-programmed at UID_BASE)
 * as 24 hex characters, so two controllers on one PC get different ports.
 */
static size_t serial_to_utf16(uint16_t *dst)
{
    static const char hex[] = "0123456789ABCDEF";
    const uint8_t *uid = (const uint8_t *)UID_BASE;
    size_t n = 0;

    for (size_t i = 0; i < 12U; i++) {
        dst[n++] = (uint16_t)hex[uid[i] >> 4];
        dst[n++] = (uint16_t)hex[uid[i] & 0x0FU];
    }
    return n;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    size_t chars;

    switch (index) {
    case STRID_LANGID:
        string_buffer[1] = 0x0409;      /* English (United States) */
        chars = 1;
        break;

    case STRID_SERIAL:
        chars = serial_to_utf16(&string_buffer[1]);
        break;

    case STRID_MANUFACTURER:
    case STRID_PRODUCT: {
        const char *str = string_descriptors[index];
        chars = strlen(str);
        if (chars > STRING_MAX_CHARS) {
            chars = STRING_MAX_CHARS;
        }
        for (size_t i = 0; i < chars; i++) {
            string_buffer[1 + i] = (uint16_t)str[i];   /* ASCII -> UTF-16 */
        }
        break;
    }

    default:
        return NULL;    /* unknown index: TinyUSB stalls the request */
    }

    string_buffer[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2U * chars + 2U));
    return string_buffer;
}
