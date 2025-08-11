#include "../inc/usbguard_interface.h"
#include "../inc/usb_info.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UsbDeviceList *usbguard_list_devices(const char *query) {
    DBusConnection *conn;
    DBusError err;
    DBusMessage *msg, *reply;
    DBusMessageIter args, arrayIter, structIter;
    unsigned int device_id;
    char *device_str;

    // Khởi tạo danh sách kết quả
    UsbDeviceList *list = calloc(1, sizeof(UsbDeviceList));

    // Khởi tạo lỗi
    dbus_error_init(&err);

    // Kết nối tới system bus
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Kết nối lỗi: %s\n", err.message);
        dbus_error_free(&err);
    }
    if (!conn) {
        fprintf(stderr, "Không thể kết nối tới system bus\n");
        free(list);
        return NULL;
    }

    // Tạo message method call
    msg = dbus_message_new_method_call(
        "org.usbguard1",
        "/org/usbguard/Devices1",
        "org.usbguard.Devices1",
        "listDevices"
    );
    if (!msg) {
        fprintf(stderr, "Không thể tạo message\n");
        free(list);
        return NULL;
    }

    // Tham số filter
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &query, DBUS_TYPE_INVALID);

    // Gửi và nhận phản hồi
    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Gọi phương thức lỗi: %s\n", err.message);
        dbus_error_free(&err);
    }
    if (!reply) {
        fprintf(stderr, "Không có phản hồi\n");
        dbus_message_unref(msg);
        free(list);
        return NULL;
    }

    // Đọc phản hồi
    if (dbus_message_iter_init(reply, &args) &&
        DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&args)) {

        dbus_message_iter_recurse(&args, &arrayIter);

        while (dbus_message_iter_get_arg_type(&arrayIter) == DBUS_TYPE_STRUCT) {
            dbus_message_iter_recurse(&arrayIter, &structIter);

            // Đọc device_id
            dbus_message_iter_get_basic(&structIter, &device_id);
            dbus_message_iter_next(&structIter);

            // Đọc device_str (chuỗi dạng "with-interface... serial=...")
            dbus_message_iter_get_basic(&structIter, &device_str);

            // Tạo đối tượng UsbDeviceInfo
            UsbDeviceInfo *dev = usb_info_create();
            char id_str[16];
            snprintf(id_str, sizeof(id_str), "%u", device_id);
            usb_info_add_property(dev, "usbguard_id", id_str);
            usb_info_add_property(dev, "raw_info", device_str);

            // Thêm vào danh sách
            list->devices = realloc(list->devices, (list->count + 1) * sizeof(UsbDeviceInfo *));
            list->devices[list->count] = dev;
            list->count++;

            dbus_message_iter_next(&arrayIter);
        }
    }

    // Giải phóng
    dbus_message_unref(msg);
    dbus_message_unref(reply);

    return list;
}

void usbguard_free_device_list(UsbDeviceList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        usb_info_free(list->devices[i]);
    }
    free(list->devices);
    free(list);
}
