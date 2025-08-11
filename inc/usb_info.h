#ifndef USB_INFO_H
#define USB_INFO_H

#include <stddef.h>

// Struct chứa thông tin thuộc tính của USB
typedef struct {
    char *key;
    char *value;
} UsbProperty;

// Struct chứa toàn bộ thông tin thiết bị USB
typedef struct {
    char *id;             // VendorID:ProductID
    char *name;           // Tên thiết bị
    char *serial;         // Serial number
    size_t property_count;
    UsbProperty *properties;
} UsbDeviceInfo;

// Hàm khởi tạo / giải phóng
UsbDeviceInfo *usb_info_create(void);
void usb_info_free(UsbDeviceInfo *info);

// Hàm cập nhật thông tin
void usb_info_set_id(UsbDeviceInfo *info, const char *vendor_id, const char *product_id);
void usb_info_set_name(UsbDeviceInfo *info, const char *name);
void usb_info_set_serial(UsbDeviceInfo *info, const char *serial);
void usb_info_add_property(UsbDeviceInfo *info, const char *key, const char *value);

// Hàm debug / in thông tin
void usb_info_print(const UsbDeviceInfo *info);

#endif
