#include "../inc/usbguard_interface.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constants for D-Bus
static const char *OBJ_PATH = "/org/usbguard1/Devices";
static const char *IFACE = "org.usbguard.Devices1";
// Try a couple of well-known bus names (some systems expose different names)
static const char *SERVICE_CANDIDATES[] = { "org.usbguard1", "org.usbguard", NULL };

UsbDeviceList *usbguard_list_devices(const char *query) {
    DBusConnection *conn;
    DBusError err;
    DBusMessage *msg = NULL, *reply = NULL;
    DBusMessageIter args, arrayIter, structIter;
    UsbDeviceList *list = NULL;

    dbus_error_init(&err);

    list = calloc(1, sizeof(UsbDeviceList));
    if (!list) return NULL;

    // Kết nối tới system bus
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "dbus_bus_get error: %s\n", err.message);
        dbus_error_free(&err);
    }
    if (!conn) {
        fprintf(stderr, "Cannot connect to system bus\n");
        free(list);
        return NULL;
    }

    // Tạo message (we'll try multiple service names if needed)
    for (const char **svc = SERVICE_CANDIDATES; *svc != NULL; svc++) {
        msg = dbus_message_new_method_call(
            *svc,
            OBJ_PATH,
            IFACE,
            "listDevices"
        );
        if (!msg) {
            fprintf(stderr, "Failed to create DBus message for service %s\n", *svc);
            continue;
        }

        // Append query string (can be "" or "allow"/"block"/"match ...")
        const char *q = query ? query : "";
        if (!dbus_message_append_args(msg, DBUS_TYPE_STRING, &q, DBUS_TYPE_INVALID)) {
            dbus_message_unref(msg);
            msg = NULL;
            continue;
        }

        // Send and wait for reply
        reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
        if (dbus_error_is_set(&err)) {
            // If service unknown, try next candidate; otherwise report and abort
            if (strstr(err.name, "ServiceUnknown") != NULL || strstr(err.name, "NameHasNoOwner") != NULL) {
                dbus_error_free(&err);
                dbus_message_unref(msg);
                msg = NULL;
                reply = NULL;
                continue; // try next candidate service
            } else {
                fprintf(stderr, "DBus call error: %s\n", err.message);
                dbus_error_free(&err);
                dbus_message_unref(msg);
                msg = NULL;
                break;
            }
        }

        // If we got a reply, break out of trying services
        if (reply) break;
        if (msg) { dbus_message_unref(msg); msg = NULL; }
    }

    if (!reply) {
        // no reply from any service
        free(list);
        return NULL;
    }

    // Parse reply: OUT a(us) devices
    if (dbus_message_iter_init(reply, &args) &&
        DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&args)) {

        dbus_message_iter_recurse(&args, &arrayIter);

        while (dbus_message_iter_get_arg_type(&arrayIter) == DBUS_TYPE_STRUCT) {
            dbus_message_iter_recurse(&arrayIter, &structIter);

            // Read u (device id)
            unsigned int device_id = 0;
            if (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_UINT32) {
                dbus_message_iter_get_basic(&structIter, &device_id);
            }
            dbus_message_iter_next(&structIter);

            // Read s (device rule string)
            char *device_rule = NULL;
            if (dbus_message_iter_get_arg_type(&structIter) == DBUS_TYPE_STRING) {
                dbus_message_iter_get_basic(&structIter, &device_rule);
            }

            // Create UsbDeviceInfo and populate some properties
            UsbDeviceInfo *dev = usb_info_create();
            if (!dev) {
                // allocation failed; skip
                dbus_message_iter_next(&arrayIter);
                continue;
            }

            // store usbguard numeric id as property
            char id_buf[32];
            snprintf(id_buf, sizeof(id_buf), "%u", device_id);
            usb_info_add_property(dev, "usbguard_id", id_buf);

            // store raw rule string as property "raw_info"
            if (device_rule)
                usb_info_add_property(dev, "raw_info", device_rule);
            else
                usb_info_add_property(dev, "raw_info", "");

            // Attempt to parse some known attributes from device_rule (simple parse)
            // Example rule contains: id 1d6b:0002 serial "..." name "..." ...
            if (device_rule) {
                // find id (VID:PID)
                char *p = strstr(device_rule, "id ");
                if (p) {
                    p += 3;
                    char vidpid[32] = {0};
                    sscanf(p, "%31s", vidpid);
                    if (strlen(vidpid) > 0) {
                        // split vendor/product
                        char *colon = strchr(vidpid, ':');
                        if (colon) {
                            *colon = '\0';
                            usb_info_set_id(dev, vidpid, colon+1);
                        }
                    }
                }

                // find name "..."
                p = strstr(device_rule, "name \"");
                if (p) {
                    p += strlen("name \"");
                    char tmp[256] = {0};
                    char *q = strchr(p, '"');
                    if (q) {
                        size_t len = q - p;
                        if (len >= sizeof(tmp)) len = sizeof(tmp)-1;
                        memcpy(tmp, p, len);
                        usb_info_set_name(dev, tmp);
                    }
                }

                // find serial "..."
                p = strstr(device_rule, "serial \"");
                if (p) {
                    p += strlen("serial \"");
                    char tmp[256] = {0};
                    char *q = strchr(p, '"');
                    if (q) {
                        size_t len = q - p;
                        if (len >= sizeof(tmp)) len = sizeof(tmp)-1;
                        memcpy(tmp, p, len);
                        usb_info_set_serial(dev, tmp);
                    }
                }
            }

            // Append to list
            UsbDeviceInfo **tmp = realloc(list->devices, (list->count + 1) * sizeof(UsbDeviceInfo *));
            if (!tmp) {
                usb_info_free(dev);
            } else {
                list->devices = tmp;
                list->devices[list->count] = dev;
                list->count++;
            }

            dbus_message_iter_next(&arrayIter);
        }
    }

    // cleanup
    if (msg) dbus_message_unref(msg);
    if (reply) dbus_message_unref(reply);

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
