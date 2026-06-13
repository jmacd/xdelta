#!/bin/sh
#
# Regression test for block-device sources (see main_blockdev_size in
# xdelta3-main.h).
#
# stat() reports st_size == 0 for a block device, so the historical code
# rejected it as non-regular and the source fell into non-seekable (FIFO)
# mode, which refuses backward source copies with:
#
#     non-seekable source in decode: XD3_INTERNAL
#
# This test attaches a real block device backed by a known image and decodes
# a delta whose copies reach backward past the decode window.  A correctly
# sized, seekable source handles this by seeking; a mis-classified FIFO source
# fails.  The test also asserts the verbose output reports a known size rather
# than "(FIFO)".
#
# Creating a block device is OS-specific and may need privileges:
#   * macOS: hdiutil (no privileges required)
#   * Linux: losetup (needs sudo)
# On any platform where a device cannot be created the test prints SKIP and
# exits 0 so it never produces a spurious failure.
#
# Usage: blockdev_test.sh [path-to-xdelta3]

set -eu

XD=${1:-}
if [ -z "$XD" ]; then
  for cand in ./build/xdelta3 ./xdelta3 build/xdelta3; do
    if [ -x "$cand" ]; then XD=$cand; break; fi
  done
fi
if [ -z "$XD" ] || [ ! -x "$XD" ]; then
  echo "blockdev_test: cannot find xdelta3 binary (pass it as \$1)" >&2
  exit 2
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/xd3-blockdev.XXXXXX")
DEV=""
OS=$(uname -s)

cleanup() {
  if [ -n "$DEV" ]; then
    case "$OS" in
      Darwin) hdiutil detach "$DEV" >/dev/null 2>&1 || true ;;
      Linux)  sudo losetup -d "$DEV" >/dev/null 2>&1 || true ;;
    esac
  fi
  rm -rf "$WORK"
}
trap cleanup EXIT INT TERM

skip() {
  echo "blockdev_test: SKIP: $1"
  exit 0
}

# --- Build the inputs --------------------------------------------------------
# A 3 MiB source, and a target that is the two halves swapped.  Decoding the
# delta therefore requires copying the first half (source offset 0) *after*
# the second half (source offset 1.5 MiB): a 1.5 MiB backward copy.
SRC="$WORK/src.bin"
TGT="$WORK/tgt.bin"
DELTA="$WORK/delta.xd"
SIZE=3145728            # 3 MiB (not a power of two)
HALF=$((SIZE / 2))

dd if=/dev/urandom of="$SRC" bs=1024 count=$((SIZE / 1024)) 2>/dev/null
dd if="$SRC" bs="$HALF" skip=1 of="$TGT" 2>/dev/null
dd if="$SRC" bs="$HALF" count=1 >>"$TGT" 2>/dev/null

# Encode with a large window so the backward match is found; decode below uses
# a small window so the copy reaches back past it.
ENC_B=4194304          # 4 MiB
DEC_B=524288           # 512 KiB (xdelta3's minimum -B)

"$XD" -e -f -B "$ENC_B" -s "$SRC" "$TGT" "$DELTA"

# Control: a regular-file source seeks and round-trips even with the small
# decode window.  If this fails the test itself is broken, so error out.
"$XD" -d -f -B "$DEC_B" -s "$SRC" "$DELTA" "$WORK/out.regular"
if ! cmp -s "$TGT" "$WORK/out.regular"; then
  echo "blockdev_test: FAIL: regular-file control decode did not round-trip" >&2
  exit 1
fi

# --- Attach the image as a block device --------------------------------------
case "$OS" in
  Darwin)
    command -v hdiutil >/dev/null 2>&1 || skip "hdiutil not available"
    DEV=$(hdiutil attach -nomount -imagekey diskimage-class=CRawDiskImage "$SRC" \
            2>/dev/null | awk 'NR==1 {print $1}') || true
    [ -n "$DEV" ] && [ -b "$DEV" ] || skip "could not attach block device via hdiutil"
    ;;
  Linux)
    command -v losetup >/dev/null 2>&1 || skip "losetup not available"
    command -v sudo >/dev/null 2>&1 || skip "sudo not available for losetup"
    sudo -n true >/dev/null 2>&1 || skip "sudo requires a password; cannot attach loop device"
    DEV=$(sudo losetup --find --show "$SRC" 2>/dev/null) || true
    [ -n "$DEV" ] && [ -b "$DEV" ] || skip "could not attach loop device"
    sudo chmod a+r "$DEV" 2>/dev/null || true
    ;;
  *)
    skip "no block-device support for $OS"
    ;;
esac

echo "blockdev_test: using device $DEV"

# --- The actual assertion ----------------------------------------------------
# Decode with the small window against the block device.  Verbose output must
# report a known size (not "(FIFO)"/"unknown"), and the output must round-trip.
LOG="$WORK/decode.log"
if ! "$XD" -d -f -v -v -B "$DEC_B" -s "$DEV" "$DELTA" "$WORK/out.device" >"$LOG" 2>&1; then
  echo "blockdev_test: FAIL: decode against block device errored:" >&2
  sed 's/^/    /' "$LOG" >&2
  exit 1
fi

if ! cmp -s "$TGT" "$WORK/out.device"; then
  echo "blockdev_test: FAIL: block-device decode did not round-trip" >&2
  exit 1
fi

if grep -qiE 'fifo|source size unknown' "$LOG"; then
  echo "blockdev_test: FAIL: block device was treated as non-seekable (FIFO):" >&2
  grep -iE 'source|fifo' "$LOG" | sed 's/^/    /' >&2
  exit 1
fi

echo "blockdev_test: PASS ($DEV sized and seekable; backward copy decoded)"
exit 0
