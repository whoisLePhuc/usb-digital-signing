#!/bin/bash
# usb_partition.sh
# Script phân vùng USB: tạo 1 vùng Reserved + 1 vùng dữ liệu

DEVICE="/dev/sdb"   # Địa chỉ thiết bị USB
RESERVED_SIZE="2MB"

# Kiểm tra quyền root
if [ "$EUID" -ne 0 ]; then
    echo "Vui lòng chạy script với quyền root (sudo)"
    exit 1
fi

echo "[*] Xóa bảng phân vùng cũ..."
parted -s "$DEVICE" mklabel gpt

echo "[*] Tạo phân vùng Reserved..."
parted -s "$DEVICE" mkpart primary ext4 1MiB $RESERVED_SIZE

echo "[*] Tạo phân vùng dữ liệu..."
parted -s "$DEVICE" mkpart DATA ext4 $RESERVED_SIZE 100%

echo "[*] Định dạng phân vùng Reserved..."
mkfs.ext4 -L RESERVED "${DEVICE}1"

echo "[*] Định dạng phân vùng DATA..."
mkfs.ext4 -L DATA "${DEVICE}2"

echo "[+] Hoàn tất phân vùng USB!"
