#ifndef CERT_GEN_H
#define CERT_GEN_H
#include "usb_info.h"

char *test_sanitize_component(const char *input);

// Function tạo private key và lưu vào file PEM
int certgen_generate_key_pem(const char *usb_key_path, int bits);

// Function tạo CSR (Certificate Signing Request) dựa trên UsbDeviceInfo và lưu vào file PEM
int certgen_generate_csr_pem(const char *usb_key_path, const char *csr_path, const UsbDeviceInfo *usbInfo);

// Function ký CRS bằng CA (ca.key và ca.crt) và lưu vào file PEM
int certgen_sign_csr_with_ca(const char *csr_path,
                             const char *ca_cert_path,
                             const char *ca_key_path,
                             const char *out_cert_path,
                             int days);

#endif // CERT_GEN_H