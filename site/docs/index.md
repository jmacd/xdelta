# Xdelta

Xdelta is a command-line tool and C library for **differential compression**:
it computes and applies binary deltas between files using the
[VCDIFF/RFC 3284](https://www.rfc-editor.org/rfc/rfc3284.txt) format. It is well
suited to distributing updates, since a delta between two similar files is
typically far smaller than the files themselves.

This is the documentation for **Xdelta version 3** (the maintained 3.2.x
series). The sources live at <https://github.com/jmacd/xdelta>.

## Quick start

Create a patch from `old_file` to `new_file`:

```sh
xdelta3 -e -s old_file new_file delta_file
```

Apply the patch to reconstruct `new_file`:

```sh
xdelta3 -d -s old_file delta_file decoded_new_file
```

Run `xdelta3 -h` for brief help and `xdelta3 test` for the built-in self test.

## Highlights

- VCDIFF (RFC 3284) encoding and decoding, with 64-bit file support on Linux,
  macOS, Windows, and other platforms.
- Optional secondary compression: `lzma` (when built with liblzma), plus the
  built-in `djw` and `fgk` coders. See
  [Better compression](better-compression.md).
- [Armor mode](armor.md): whole-file [BLAKE3](https://github.com/BLAKE3-team/BLAKE3)
  verification embedded in the delta, on by default, so applying a patch with
  the wrong source fails fast with a clear message.
- A stable streaming C API for embedding. See the
  [Programming guide](programming-guide.md).

## Where to go next

- [Command-line syntax](commandline.md) — the full option reference.
- [Armor mode](armor.md) — how whole-file verification works.
- [Tuning the memory budget](tuning-memory.md) — `-B`, `-W`, `-I`, `-P`.
- [Programming guide](programming-guide.md) — the encode/decode API.
- [Licensing](licensing.md) — the maintained branches are Apache 2.0.

!!! note "History"
    Xdelta 1.x (first released in 1999) and the original GPL-licensed Xdelta 3
    are no longer maintained here. The GPL line lives at
    <https://github.com/jmacd/xdelta-gpl>.
