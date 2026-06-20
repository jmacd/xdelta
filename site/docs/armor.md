# Armor mode

**Armor mode** embeds [BLAKE3](https://github.com/BLAKE3-team/BLAKE3) digests of
the source and target files in the VCDIFF application header and verifies them
when applying a delta. It turns the classic late, ambiguous
[target-window checksum mismatch](checksum-mismatch.md) into an immediate, clear
error when the wrong source is supplied.

Armor is **on by default**. Pass `-a` to disable it.

## What it does

When encoding, xdelta3 reads the source and target in full to compute their
BLAKE3 digests and records them in the application header using a
`name#blake3hex` syntax. When decoding:

- The **source is verified up front**, before applying the delta. A wrong
  source fails fast with a clear message instead of a late window-checksum
  error.
- The reconstructed **target is verified afterward**.
- If the supplied source already matches the delta's *target* digest — i.e. the
  patch is already applied — decode reports "already up to date" and exits with
  status `2`.

```sh
# Encode (armor on by default); both files must be seekable regular files.
xdelta3 -e -s old_file new_file delta_file

# Decode verifies old_file before applying and new_file afterward.
xdelta3 -d -s old_file delta_file decoded_new_file
```

## Requirements and behavior

- **Seekable inputs.** Encoding needs to read both source and target twice, so
  both must be seekable (regular) files. If a source is a stream (not seekable),
  the decoder cannot verify it up front, so it prints a warning and proceeds.
- **Logical content.** The digests cover the logical (decompressed) content
  xdelta3 processes, not the raw on-disk bytes, so armor composes correctly with
  [external compression](external-compression.md) of inputs and recompression
  of output.
- **`merge`.** Merging an armored chain of deltas verifies the chain when every
  input is armored, and the merged delta is re-armored.

## Disabling armor (`-a`)

Pass `-a` to disable armor. This restores the legacy application-header format
and the non-seekable streaming behavior — useful for piped input or when you do
not want the extra read pass.

!!! warning "Interoperating with legacy xdelta3"
    Legacy xdelta3 builds (and builds without armor) read the armored `name#hex`
    application-header values as literal filenames. To apply an armored delta
    with such a build, specify the source and output explicitly with `-s` and an
    output filename.

## Building without armor

Armor is controlled at build time by the CMake option `XD3_ARMOR` (default
`ON`), which fetches and statically links BLAKE3. Configure with
`-DXD3_ARMOR=OFF` to compile armor out entirely; the resulting tool behaves as
if `-a` were always given and has no BLAKE3 dependency.

## See also

- [Command-line syntax](commandline.md) — the `-a` option and exit codes.
- ["Target window checksum mismatch" explained](checksum-mismatch.md) — the
  error armor is designed to pre-empt.
