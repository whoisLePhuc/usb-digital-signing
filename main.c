#include "usb_info.h"
#include "usbguard_interface.h"
#include "cert_gen.h"
#include "embed_cert.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define USB_SCRIPT_PATH "usbPartition.sh"
#define USB_SIGNATURE_PATH "output/usb_cert.pem"
#define USB_DEVICE "/dev/sdb"

int main() {
    embed_cert(USB_SCRIPT_PATH, USB_DEVICE, USB_SIGNATURE_PATH);
}


/*
static void print_short_line(const UsbDeviceInfo *d, size_t idx) {
    const char *id = usb_info_get_property(d, "usbguard_id");
    const char *raw = usb_info_get_property(d, "raw_info");
    // print first line: index, usbguard id, and trimmed raw_info preview
    printf("[%2zu] id=%s - ", idx, id ? id : "(?)");
    if (raw) {
        // print up to 80 chars of raw
        for (int i = 0; i < 80 && raw[i] && raw[i] != '\n'; i++) putchar(raw[i]);
        if ((int)strlen(raw) > 80) printf("...");
    }
    putchar('\n');
}

int main(void) {
    // get all devices
    UsbDeviceList *Devicelist = usbguard_list_devices("match"); // empty query -> all
    UsbDeviceInfo *UsbInfo = usb_info_create();
    if (!Devicelist) {
        fprintf(stderr, "Failed to obtain device list from USBGuard.\n");
        return 1;
    }

    if (Devicelist->count == 0) {
        printf("No USB devices found (according to usbguard).\n");
        usbguard_free_device_list(Devicelist);
        return 0;
    }

    printf("Found %zu device(s):\n", Devicelist->count);
    for (size_t i = 0; i < Devicelist->count; i++) {
        print_short_line(Devicelist->devices[i], i);
    }

    // Interactive selection loop
    while (1) {
        printf("\nEnter device index to view details (q to quit): ");
        char buf[64];
        if (!fgets(buf, sizeof(buf), stdin)) break;

        // trim leading spaces
        char *p = buf;
        while (*p && isspace((unsigned char)*p)) p++;

        if (*p == 'q' || *p == 'Q') break;

        char *endptr;
        long idx = strtol(p, &endptr, 10);
        if (p == endptr) {
            printf("Invalid input. Enter a number or 'q'.\n");
            continue;
        }
        if (idx < 0 || (size_t)idx >= Devicelist->count) {
            printf("Index out of range. Valid: 0..%zu\n", Devicelist->count - 1);
            continue;
        }

        printf("\n--- Device %ld details ---\n", idx);
        UsbInfo = Devicelist->devices[idx];
        usb_info_print(UsbInfo);
        certgen_generate_key_pem("output/usb.key", 2048);
        certgen_generate_csr_pem("output/usb.key", "output/usb.csr", UsbInfo);
        certgen_sign_csr_with_ca("output/usb.csr", "cert/ca.crt", "cert/ca.key", "output/usb_cert.pem", 365);
    }

    usbguard_free_device_list(Devicelist);
    printf("Goodbye.\n");
    return 0;
}
*/








