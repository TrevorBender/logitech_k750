#include <linux/hid.h>
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (void)
{
    libusb_context * ctx;
    libusb_init (&ctx);
    libusb_set_debug (ctx, 3);

    libusb_device_handle * device_handle = libusb_open_device_with_vid_pid (ctx, 0x046d, 0xc52b);

    libusb_device * device = libusb_get_device (device_handle);

    struct libusb_device_descriptor desc;

    libusb_get_device_descriptor (device, &desc);

    for (uint8_t config_index = 0; config_index < desc.bNumConfigurations; config_index++) {
        struct libusb_config_descriptor * config;
        libusb_get_config_descriptor (device, config_index, &config);

        int iface_index = 2;
        const struct libusb_interface * iface = & config->interface[iface_index];

        for (int altsetting_index = 0; altsetting_index < iface->num_altsetting; altsetting_index++) {
            const struct libusb_interface_descriptor * iface_desc = & iface->altsetting[altsetting_index];

            if (iface_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
                libusb_detach_kernel_driver (device_handle, iface_index);
                libusb_claim_interface (device_handle, iface_index);
                unsigned char ret[65535];
                unsigned char payload[] = "\x10\x01\x09\x03\x78\x01\x00";

                if (libusb_control_transfer(device_handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                            HID_REQ_SET_REPORT,
                            0x0210, iface_index, payload, sizeof (payload) - 1, 10000)) {
                    int actual_length = 0;
                    while (!(actual_length == 20 && ret[0] == 0x11 && ret[2] == 0x09 && ret[3] == 0x10))
                        libusb_interrupt_transfer (device_handle,
                                iface_desc->endpoint[0].bEndpointAddress,
                                ret, sizeof (ret), &actual_length, 100000);
                    uint16_t lux = ret[5] << 8 | ret[6];

                    printf ("Charge: %d %%\nLight: %d lux\n", ret[4], lux);
                }

                libusb_release_interface (device_handle, iface_index);
                libusb_attach_kernel_driver (device_handle, iface_index);
            }
        }
    }

    libusb_close (device_handle);
    libusb_exit (ctx);

    return EXIT_SUCCESS;
}
