# "Target window checksum mismatch" explained

This explains the `target window checksum mismatch: XD3_INVALID_INPUT` error.

!!! tip "First, the short answer"
    **This error almost always means you supplied the wrong source file.**
    Verify the source with a strong checksum (for example `sha256sum`, or
    BLAKE3) and confirm it matches the source used to create the delta.

!!! note "Armor mode pre-empts this error"
    Since the addition of [armor mode](armor.md) (on by default), an armored
    delta carries whole-file BLAKE3 digests, so decoding **verifies the source
    up front** and fails immediately with a clear message rather than a late
    per-window checksum error. You will only see the window-checksum error
    below for deltas produced (or applied) with armor disabled (`-a`), or for
    non-seekable sources that cannot be verified up front. If you control the
    encoding side, keeping armor enabled is the best way to avoid this error.

## Why the error is reported so late (without armor)

The xdelta3 command-line tool supports streaming both the source and the target
inputs, so historically it could not verify a whole-file checksum before
beginning. Because the VCDIFF format is window-oriented, integrity is checked at
the granularity of a single window. As a result, a decoder using the wrong
source may partially decode the input before a window checksum fails.

Properly encoding and verifying a whole-file checksum as part of the patch would
require two passes over the data during both encoding and decoding, and the
VCDIFF format only allows metadata in the header — which is written before the
entire source can be read. Armor mode solves this at the application-header
level by recording BLAKE3 digests and reading the inputs in full; see
[Armor mode](armor.md).

## Other possible causes

The target-window checksum is a catch-all. Besides the wrong source file (the
only "normal" cause), an incorrect encoding (a bug), hardware failure, or an
internal error could trigger it. Check the source file first.
