#include "usb_info.h"
#include "usbguard_interface.h"
#include "cert_gen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


/* Test code for sanitizing USB device information components
 * This code demonstrates how to sanitize USB device information
 * by removing special characters from the id, name, and serial fields.
 */
/*
int main() {
    // Dữ liệu USB giả lập với ký tự đặc biệt
    UsbDeviceInfo dev = {
        .id = "1d6b:0002@#$",
        .name = "My USB Device 2025!!",
        .serial = "SN 123 456*(&"
    };

    printf("Before sanitize:\n");
    printf("  id     = %s\n", dev.id);
    printf("  name   = %s\n", dev.name);
    printf("  serial = %s\n", dev.serial);

    char *clean_id = test_sanitize_component(dev.id);
    char *clean_name = test_sanitize_component(dev.name);
    char *clean_serial = test_sanitize_component(dev.serial);

    printf("\nAfter sanitize:\n");
    printf("  id     = %s\n", clean_id);
    printf("  name   = %s\n", clean_name);
    printf("  serial = %s\n", clean_serial);

    free(clean_id);
    free(clean_name);
    free(clean_serial);

    return 0;
}
*/

/* Test code for printing USB device information
 * This code demonstrates how to create a USB device info structure,
 * set its properties, and print the information.
 */
/*
int main() {
    UsbDeviceInfo *dev = usb_info_create();

    usb_info_set_id(dev, "048d", "1234");
    usb_info_set_name(dev, "My USB Flash");
    usb_info_set_serial(dev, "SN987654321");

    usb_info_add_property(dev, "with-interface", "08:06:50");
    usb_info_add_property(dev, "speed", "480Mbps");

    usb_info_print(dev);

    usb_info_free(dev);
    return 0;
}
*/

/* Test code for listing USB devices using usbguard
 * This code demonstrates how to list USB devices and print their information.
 */
/*
int main() {
    UsbDeviceList *list = usbguard_list_devices("");
    if (!list) {
        printf("Không lấy được danh sách USB.\n");
        return 1;
    }

    printf("Có %zu thiết bị:\n", list->count);
    for (size_t i = 0; i < list->count; i++) {
        usb_info_print(list->devices[i]);
    }

    usbguard_free_device_list(list);
    return 0;
}
*/

