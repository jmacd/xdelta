#!/bin/sh
#
# Regression test for external-compression level detection (see
# main_detect_level_* and main_recompress_output in xdelta3-main.h).
#
# When xdelta3 automatically decompresses a compressed input and later
# automatically recompresses the decoded output, it must reproduce the
# *original* compression level so the recompressed bytes match the original
# compressed file.  The level is detected from the input header at encode time,
# carried in the VCDIFF application header as "<ident><level>" (e.g. "B9"), and
# passed back to the recompressor at decode time.
#
# bzip2 is used because its output is fully deterministic (no timestamp,
# filename, or OS fields) and its block-size level (1..9) is exactly recoverable
# from the "BZh<n>" header, so a byte-identical round-trip is a reliable check
# across environments.  Without level detection, the level-1 and level-5 cases
# would be recompressed at bzip2's default level and would not match.
#
# Usage: recompress_level_test.sh [path-to-xdelta3]

set -eu

XD=${1:-}
if [ -z "$XD" ]; then
  for cand in ./build/xdelta3 ./xdelta3 build/xdelta3; do
    if [ -x "$cand" ]; then XD=$cand; break; fi
  done
fi
if [ -z "$XD" ] || [ ! -x "$XD" ]; then
  echo "recompress_level_test: cannot find xdelta3 binary (pass it as \$1)" >&2
  exit 2
fi

skip() {
  echo "recompress_level_test: SKIP: $1"
  exit 0
}

if ! command -v bzip2 >/dev/null 2>&1; then
  skip "bzip2 not available"
fi

WORK=$(mktemp -d "${TMPDIR:-/tmp}/xd3-recomp.XXXXXX")
trap 'rm -rf "$WORK"' EXIT INT TERM

# --- Build deterministic inputs ----------------------------------------------
# A base file and a target that differs from it, both compressible.  The exact
# contents do not matter as long as they are reproducible within the run.
i=0
while [ "$i" -lt 4000 ]; do
  printf 'xdelta3 recompress level regression line %d\n' "$i"
  i=$((i + 1))
done > "$WORK/base.bin"
cp "$WORK/base.bin" "$WORK/target.bin"
printf 'appended changes that make the target differ from the base\n' \
  >> "$WORK/target.bin"

fail=0
for L in 1 5 9; do
  bzip2 -"$L" -c "$WORK/base.bin"   > "$WORK/base.bz2"
  bzip2 -"$L" -c "$WORK/target.bin" > "$WORK/target.bz2"

  # Encode a delta from the compressed source to the compressed target.  The
  # encoder transparently decompresses both and records the level.
  "$XD" -f -e -s "$WORK/base.bz2" "$WORK/target.bz2" "$WORK/delta.xd3" \
    2>"$WORK/enc.log"

  # Decode: xdelta3 must recompress the output with bzip2 at the same level.
  "$XD" -f -d -s "$WORK/base.bz2" "$WORK/delta.xd3" "$WORK/out.bz2" \
    2>"$WORK/dec.log"

  if cmp -s "$WORK/target.bz2" "$WORK/out.bz2"; then
    echo "recompress_level_test: level $L byte-identical OK"
  else
    echo "recompress_level_test: level $L MISMATCH (recompressed output differs)" >&2
    fail=1
  fi
done

# --- -G opt-out: legacy header, default-level recompression -----------------
# bzip2 defaults to -9, so a level-1 input encoded with -G must NOT round-trip
# byte-identically (the level was intentionally dropped), yet the decoded
# content must still be correct and decodable by older xdelta3 versions.
bzip2 -1 -c "$WORK/base.bin"   > "$WORK/base.bz2"
bzip2 -1 -c "$WORK/target.bin" > "$WORK/target.bz2"

"$XD" -G -f -e -s "$WORK/base.bz2" "$WORK/target.bz2" "$WORK/deltaG.xd3" \
  2>"$WORK/encG.log"
"$XD" -f -d -s "$WORK/base.bz2" "$WORK/deltaG.xd3" "$WORK/outG.bz2" \
  2>"$WORK/decG.log"

if cmp -s "$WORK/target.bz2" "$WORK/outG.bz2"; then
  echo "recompress_level_test: -G unexpectedly preserved the level" >&2
  fail=1
else
  echo "recompress_level_test: -G dropped the level (legacy header) OK"
fi

bunzip2 -c "$WORK/outG.bz2" > "$WORK/outG.bin"
if cmp -s "$WORK/target.bin" "$WORK/outG.bin"; then
  echo "recompress_level_test: -G decoded content correct OK"
else
  echo "recompress_level_test: -G decoded content WRONG" >&2
  fail=1
fi

if [ "$fail" -ne 0 ]; then
  echo "recompress_level_test: FAIL" >&2
  exit 1
fi

echo "recompress_level_test: PASS"
exit 0
