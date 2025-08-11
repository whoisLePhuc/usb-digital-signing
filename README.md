# USB Certificate Generator

A lightweight C-based tool for securely provisioning USB storage devices with embedded X.509 certificates. The system generates a certificate signed by an internal Certificate Authority (CA) and embeds it into a reserved partition on the USB.

This project is designed for environments requiring trusted device authentication, such as industrial systems, secure boot chains, and hardware access control.

---

## ✨ Features

- Extract USB identifiers (Serial Number, VendorID, ProductID)
- Generate X.509 certificates using OpenSSL
- Sign certificates with an internal CA
- Embed certificates into reserved USB partitions
- Modular, maintainable C codebase

---

## 📂 Directory Structure

```
    usb-cert-generator/
    ├── cert/                         # Chứa chứng chỉ CA
	│   ├── ca.crt                    # CA certificate (public)
	│   └── ca.key                    # CA private key (secret)
    │
    ├── output/             # Generated keys and certificates
	│   ├── usb.key
	│   ├── usb.csr
	│   └── usb_cert.pem              # Chứng chỉ USB được tạo
    │
    ├── src/                # C source code files
    │   ├── usb_info.c      # Lấy thông tin USB
    │   ├── cert_gen.c      # Sinh chứng chỉ bằng openssl
    │   ├── usbguard_interface.c  # Giao tiếp với USBguard 
    │   └── embed_cert.c    # Nhúng cert vào USB
    │
    ├── inc/                # C source code files
    │   ├── usb_info.h
    │   ├── cert_gen.h
    │   ├── usbguard_interface.h
    │   └── embed_cert.h
    │
    ├── main.c
    ├── Makefile
    └── README.md
```

---

## 🚀 Quick Start

### 1. Generate your internal CA (if not available)
```bash
mkdir cert
cd cert
openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -out ca.crt
```

### 2. Build the tool
```bash
cd src
make
```

### 3. Run the tool (replace `/dev/sdX` and `/dev/sdX1` with your USB device paths)
```bash
sudo ./usb_cert_generator
```

> ⚠️ This tool writes to your USB. Make sure to select the correct device to avoid data loss.

---

## 📌 Requirements

- Linux environment
- `gcc` compiler
- `openssl` CLI tools
- `udevadm` utility

---

## 📜 License

MIT License

---

## 🤝 Contributing

Pull requests are welcome! For significant changes, please open an issue first to discuss the improvements.
