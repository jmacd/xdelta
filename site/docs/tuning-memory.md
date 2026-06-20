# Tuning the memory budget

There are five memory-cost parameters that determine compression performance,
independent of compression level. The code was designed to work with a fixed
memory budget regardless of input size; poor compression can result from an
insufficient budget. Defaults are automatically lowered for small files.

!!! note "Units"
    All flags are set in **bytes**. For example, a 512 MB source buffer is
    `-B536870912`.

## Source buffer size (`-B`)

The encoder uses a buffer for the source input (size set by `-B`). To ensure the
source is read sequentially with no backward seeks, the encoder keeps the source
*horizon* half the source buffer size ahead of the input position. A source copy
will not be found if it lies more than half the source buffer size away from its
absolute position in the input stream.

For large files, `-B` may need to be raised. The default is 64 MB, which means
data should not shift more than 32 MB — that is, no more than 32 MB should be
added or removed relative to the source. The minimum is 16 KB. The source file
is read into the buffer; it is not `mmap`ed (Xdelta 1.x used `mmap()`).

## Input window size (`-W`)

The input window size (`-W`) determines how much input is compressed in a single
VCDIFF window. Smaller windows have higher compression cost and take less memory
to decode. Larger windows give better compression, but only up to a point, since
large-window addresses take more bits to encode. The default is 8 MB, the
minimum 16 KB, and the maximum 16 MB.

## Instruction buffer size (`-I`)

The instruction buffer stores potential, possibly overlapping, copy instructions
while the encoder looks ahead. Set its size with `-I size` (default 32K slots);
`-I 0` selects an unlimited buffer.

An unencoded instruction occupies 28 bytes, so a bounded buffer has advantages.
On the other hand, the minimum and maximum source addresses must be decided
before encoding the first instruction, so letting the buffer fill before a
window finishes can hurt compression for the rest of the window.

## Compression duplicates size (`-P`)

The compressor uses an array of duplicate positions (`-P`) to find better
matches in the **target** (not the source). This should be less than or equal to
the input window size (`-W`). The default is 256K slots.

## Compression level (`-0` … `-9`)

The compression level sizes two internal data structures. `-9` uses about four
times as much memory as `-1`.

## Decoder memory requirements

To decode, `-B` and `-W` are used much as for encoding. For the source buffer,
the decoder uses the smaller of `-B` or the source file size. Setting `-B`
smaller than the value used to encode causes seeking, served by an LRU cache of
blocks.

The input buffer is sized according to `-W`. In addition, the decoder allocates
three buffers for the data, address, and instruction sections of a VCDIFF
window; their sizes depend on the compressed size of a window and therefore on
the encoder's `-W`. If secondary compression is used, an extra set of buffers is
allocated for each secondary-compressed section.

In summary, decoding uses `-B` bytes for the source buffer, plus `-W` bytes for
its input buffer, plus three or six buffers totaling no more than one or two
times the encoder's `-W` (depending on secondary compression).
