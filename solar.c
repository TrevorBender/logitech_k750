/**
 * Taken and modified from Julien Danjou's blog:
 * http://julien.danjou.info/blog/2012/logitech-k750-linux-support
 *
 * Used usbmon to determine control packet as it was different.
 */
#include <linux/hid.h>
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dbg.h"

#define FULL_DEBUG (3)

static unsigned char control_packet[] = "\x10\x01\x09\x03\x78\x01\x00";

static void perform_usb_transfer (libusb_device_handle * device_handle, int iface_index, uint8_t endpoint_address);

int main (void)
{
    libusb_context * ctx;
    libusb_init (&ctx);
    libusb_set_debug (ctx, FULL_DEBUG);

    libusb_device_handle * device_handle = libusb_open_device_with_vid_pid (ctx, 0x046d, 0xc52b);
    check (device_handle, "Error getting device handle");

    libusb_device * device = libusb_get_device (device_handle);

    struct libusb_device_descriptor desc;

    libusb_get_device_descriptor (device, &desc);

    struct libusb_config_descriptor * config;
    libusb_get_config_descriptor (device, 0, &config);

    int iface_index = 2;
    const struct libusb_interface * iface = & config->interface[iface_index];

    const struct libusb_interface_descriptor * iface_desc = & iface->altsetting[0];

    perform_usb_transfer (device_handle, iface_index, iface_desc->endpoint[0].bEndpointAddress);

    libusb_close (device_handle);
    libusb_exit (ctx);

    return EXIT_SUCCESS;

error:
    if (device_handle) libusb_close (device_handle);
    libusb_exit (ctx);
}

void perform_usb_transfer (libusb_device_handle * device_handle, int iface_index, uint8_t endpoint_address)
{
    if (libusb_kernel_driver_active (device_handle, iface_index)) {
        int result = libusb_detach_kernel_driver (device_handle, iface_index);
        check (!result, "Unable to detach kernel driver");
    }
    int result = libusb_claim_interface (device_handle, iface_index);
    check (!result, "Unable to claim interface");

    unsigned char ret[65535];

    if (libusb_control_transfer(device_handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                HID_REQ_SET_REPORT,
                0x0210, iface_index, control_packet, sizeof (control_packet) - 1, 10000)) {
        int actual_length = 0;
        while (!(actual_length == 20 && ret[0] == 0x11 && ret[2] == 0x09 && ret[3] == 0x10))
            libusb_interrupt_transfer (device_handle,
                    endpoint_address,
                    ret, sizeof (ret), &actual_length, 100000);
        uint16_t lux = ret[5] << 8 | ret[6];

        printf ("Charge: %d %%\nLight: %d lux\n", ret[4], lux);
    } else
        log_err ("Unable to send control");

error:
    libusb_release_interface (device_handle, iface_index);
    libusb_attach_kernel_driver (device_handle, iface_index);
}
