#!/usr/bin/env bash
# usb_write_raw_sig.sh
# Tạo partition (1MiB reserved + phần còn lại data),
# rồi ghi raw PEM (signature) trực tiếp vào reserved partition (sdX1) bằng dd.
# HỖ TRỢ NVMe/mmcblk naming.
#
# Usage:
#   sudo ./usb_write_raw_sig.sh /dev/sdX /path/to/signature.pem [--format-data]
#
set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "Usage: sudo $0 <device> <signature_pem> [--format-data]"
  exit 1
fi

DEV="$1"
SIG="$2"
FORMAT_DATA="${3:-}"

# Check root
if [[ "$EUID" -ne 0 ]]; then
  echo "Run as root (sudo)."
  exit 2
fi

# Basic checks
if [[ ! -b "$DEV" ]]; then
  echo "ERROR: $DEV is not a block device."
  exit 3
fi
if [[ ! -f "$SIG" ]]; then
  echo "ERROR: signature file $SIG not found."
  exit 4
fi

# Confirm device
echo "About to wipe and partition device: $DEV"
lsblk "$DEV"
read -p "Type the device path to CONFIRM (e.g. $DEV): " CONF
if [[ "$CONF" != "$DEV" ]]; then
  echo "Confirmation mismatch. Abort."
  exit 5
fi

# ==== NEW: Unmount all existing partitions of the device ====
echo "[0/6] Unmounting existing partitions of $DEV..."
for p in $(lsblk -ln -o NAME "$DEV" | tail -n +2); do
    umount "/dev/$p" 2>/dev/null || true
done
sync

# Resolve partition names depending on device type (nvme/mmcblk need 'p' before number)
base=$(basename "$DEV")
if [[ "$base" == nvme* || "$base" == mmcblk* ]]; then
  P1="${DEV}p1"
  P2="${DEV}p2"
else
  P1="${DEV}1"
  P2="${DEV}2"
fi

echo "[1/6] Wiping partition table (sgdisk --zap-all)..."
if command -v sgdisk >/dev/null 2>&1; then
  sgdisk --zap-all "$DEV"
else
  dd if=/dev/zero of="$DEV" bs=1M count=2 status=progress conv=fsync
fi

echo "[2/6] Create GPT + partitions (1MiB reserved -> 2MiB, data 2MiB->end)..."
parted -s "$DEV" mklabel gpt
parted -s "$DEV" unit MiB mkpart primary 1 2
parted -s "$DEV" unit MiB mkpart primary 2 100%
parted -s "$DEV" name 1 "USB_SIG"
parted -s "$DEV" name 2 "USB_DATA"

echo "[3/6] Inform kernel and wait for /dev entries..."
partprobe "$DEV" || true
udevadm settle --exit-if-exists="$P1" || sleep 1

for i in {1..20}; do
  if [[ -b "$P1" ]]; then break; fi
  sleep 0.5
done
if [[ ! -b "$P1" ]]; then
  echo "ERROR: partition device $P1 not found after parted."
  exit 6
fi

echo "[4/6] Zero (wipe) the reserved partition (1 MiB) to remove old data..."
dd if=/dev/zero of="$P1" bs=1M count=1 status=progress conv=fsync

SIG_SIZE_BYTES=$(stat -c%s "$SIG")
MAX_BYTES=$((1024*1024))  # 1 MiB

if (( SIG_SIZE_BYTES > MAX_BYTES )); then
  echo "ERROR: signature file is larger than 1 MiB ($SIG_SIZE_BYTES bytes)."
  exit 7
fi

echo "[5/6] Prepare 1MiB blob (pad signature to 1MiB) and write to $P1..."
TMP_BLOB=$(mktemp /tmp/reserved_blob.XXXXXX)
dd if=/dev/zero of="$TMP_BLOB" bs=1M count=1 status=none
dd if="$SIG" of="$TMP_BLOB" conv=notrunc status=none
dd if="$TMP_BLOB" of="$P1" bs=1M conv=fsync status=progress
rm -f "$TMP_BLOB"

echo "[6/6] Verify written content by reading back and comparing..."
TMP_READ=$(mktemp /tmp/read_sig.XXXXXX)
dd if="$P1" of="$TMP_READ" bs=1M count=1 status=none
if cmp -n "$SIG_SIZE_BYTES" -- "$SIG" "$TMP_READ"; then
  echo "VERIFY: OK — signature written correctly to $P1"
  rm -f "$TMP_READ"
else
  echo "VERIFY: MISMATCH — signature differs!"
  exit 8
fi

# NEW: Format Data Partition if requested
if [[ "$FORMAT_DATA" == "--format-data" ]]; then
  echo "Formatting data partition $P2..."
  mkfs.vfat -F 32 "$P2"
  echo "Data partition formatted (FAT32)."
else
  echo "Data partition $P2 left unformatted."
fi

echo "Done. Reserved partition $P1 contains the raw PEM (padded to 1MiB)."
