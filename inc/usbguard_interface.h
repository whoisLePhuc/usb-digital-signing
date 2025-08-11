#ifndef USBGUARD_INTERFACE_H
#define USBGUARD_INTERFACE_H

#include "usb_info.h"
#include <stddef.h>

// Danh sách thiết bị USB đọc từ USBGuard
typedef struct {
    UsbDeviceInfo **devices;
    size_t count;
} UsbDeviceList;

// Lấy danh sách thiết bị từ USBGuard (filter: "match", "allow", "block")
// Trả về con trỏ đến UsbDeviceList (caller responsible to free via usbguard_free_device_list).
UsbDeviceList *usbguard_list_devices(const char *query);

// Giải phóng danh sách
void usbguard_free_device_list(UsbDeviceList *list);

#endif // USBGUARD_INTERFACE_H
