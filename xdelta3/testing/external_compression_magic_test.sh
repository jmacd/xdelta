#!/bin/sh

set -eu

XD=${1:-}
if [ -z "$XD" ] || [ ! -x "$XD" ]; then
  echo "external_compression_magic_test: xdelta3 binary is required" >&2
  exit 2
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/xd3-magic.XXXXXX")
trap 'rm -rf "$WORK"' EXIT INT TERM

: > "$WORK/source"

check_raw() {
  name=$1
  bytes=$2

  printf '%b' "$bytes" > "$WORK/$name"
  "$XD" -f -e -s "$WORK/source" "$WORK/$name" "$WORK/$name.delta"
  "$XD" -f -d -s "$WORK/source" "$WORK/$name.delta" "$WORK/$name.out"
  cmp "$WORK/$name" "$WORK/$name.out"
}

# Partial or mismatched XZ signatures must remain ordinary input.
check_raw short3 '\375\067\000'
check_raw short4 '\375\067\172\000'
check_raw short5 '\375\067\172\130\000'

# A complete XZ signature must still select external decompression.
if command -v xz >/dev/null 2>&1; then
  printf 'complete xz signature\n' > "$WORK/plain"
  xz -c "$WORK/plain" > "$WORK/plain.xz"
  "$XD" -f -e -s "$WORK/source" "$WORK/plain.xz" "$WORK/xz.delta" \
    2> "$WORK/xz.log"
  grep -q "externally compressed input: xz " "$WORK/xz.log"
  "$XD" -f -R -d -s "$WORK/source" "$WORK/xz.delta" "$WORK/xz.out"
  cmp "$WORK/plain" "$WORK/xz.out"
fi

echo "external_compression_magic_test: PASS"
