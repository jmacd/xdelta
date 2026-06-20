# Non-seekable source files

How to compress when the **source** is a stream, named pipe, or FIFO rather
than a regular file.

## Overview

It is possible to read the source file from a FIFO or named pipe. The encoder is
designed not to seek backward in the source, but this has implications for the
decoder's memory budget.

## Details

The source window size (set by `-B`) used by the **decoder** must be **greater
than or equal to** the source window size used by the **encoder**. If a copy
references source data further back than the decoder's window allows, you will
see:

```
xdelta3: non-seekable source: copy is too far back (try raising -B): XD3_INTERNAL
```

Raise `-B` (see [Tuning the memory budget](tuning-memory.md)) so the decoder
keeps enough of the source in memory.

This non-seekable mode is the default for
[external compression](external-compression.md): when you pass a compressed
source file on the command line, xdelta3 forks the decompression command and
reads the source through a pipe.

!!! note "Armor mode requires a seekable source"
    [Armor mode](armor.md) (on by default) reads the source in full to compute
    and verify its BLAKE3 digest, which requires a seekable (regular) file. With
    a non-seekable source the decoder cannot verify it up front, so it warns and
    proceeds; pass `-a` to disable armor for fully streaming use.
