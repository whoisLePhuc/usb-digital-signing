#include "../inc/usb_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UsbDeviceInfo *usb_info_create(void) {
    UsbDeviceInfo *info = calloc(1, sizeof(UsbDeviceInfo));
    return info;
}

void usb_info_free(UsbDeviceInfo *info) {
    if (!info) return;

    free(info->id);
    free(info->name);
    free(info->serial);

    for (size_t i = 0; i < info->property_count; i++) {
        free(info->properties[i].key);
        free(info->properties[i].value);
    }
    free(info->properties);

    free(info);
}

void usb_info_set_id(UsbDeviceInfo *info, const char *vendor_id, const char *product_id) {
    free(info->id); // tránh leak memory
    size_t len = strlen(vendor_id) + strlen(product_id) + 2; // +1 cho ':' và +1 cho '\0'
    info->id = malloc(len);
    snprintf(info->id, len, "%s:%s", vendor_id, product_id);
}

void usb_info_set_name(UsbDeviceInfo *info, const char *name) {
    free(info->name);
    info->name = strdup(name);
}

void usb_info_set_serial(UsbDeviceInfo *info, const char *serial) {
    free(info->serial);
    info->serial = strdup(serial);
}

void usb_info_add_property(UsbDeviceInfo *info, const char *key, const char *value) {
    info->properties = realloc(info->properties, (info->property_count + 1) * sizeof(UsbProperty));
    info->properties[info->property_count].key = strdup(key);
    info->properties[info->property_count].value = strdup(value);
    info->property_count++;
}

void usb_info_print(const UsbDeviceInfo *info) {
    if (!info) return;

    printf("USB Device Info:\n");
    printf("  ID: %s\n", info->id ? info->id : "(null)");
    printf("  Name: %s\n", info->name ? info->name : "(null)");
    printf("  Serial: %s\n", info->serial ? info->serial : "(null)");

    printf("  Properties (%zu):\n", info->property_count);
    for (size_t i = 0; i < info->property_count; i++) {
        printf("    %s: %s\n", info->properties[i].key, info->properties[i].value);
    }
}
