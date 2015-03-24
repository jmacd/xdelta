# Introduction #

There are five memory-cost parameters that determine compression performance, independent of compression level. The code was designed to work with a fixed memory budget, regardless of input size: bad compression performance may result from an insufficient memory budget.

Defaults are automatically lowered for small files.

Note: _all flags are set in bytes, so for example to set a 512MB source buffer you must pass -B536870912._

# Source buffer size #

The encoder uses a buffer for the source input (of size set by the command-line flag -B). To ensure the source input is read sequentially, with no backward seeks, the encoder maintains the source _horizon_ at half the source buffer size ahead of the input position. A source copy will not be found if it lies more than half the source buffer size away from its absolute position in the input stream.

For large files, -B may need to be raised. The default is 64MB.  This means data should not shift more than 32MB, that is, not more than 32MB should be added or removed from the source.

The minimum value of -B is 16KB.

The source file is not mmaped, it is read into the source buffer (Xdelta-1.x used mmap()).

# Input window size #

The input window size (set by -W) determines how much of the input is compressed in a single VCDIFF window.  Smaller windows have higher compression cost and take less memory to decode.  Larger windows have better compression but only up to a point, because large-window addresses take more bits to encode.

The default is 8MB.  The minumim value is 16KB. The maximum value is 16MB.

# Instruction buffer size #

The instruction buffer saves potential encodings, (possibly) overlapping copy instructions, while it looks ahead in the input. The size of this buffer can be set on the command line via `-I size`, the default is 32K slots. An unlimited instruction buffer is supported with `-I 0`.

An unencoded instruction occupies 28 bytes, so limiting buffer size does have advantages. On the other hand, the minimum/maximum source address have to be decided before encoding the first instruction, so letting the buffer fill before finishing a window can cause bad compression for the remaining input window.

# Compression duplicates size #

The compressor uses an array of duplicate positions (set by -P) to find better matches in the target (not the source). This should be less than or equal to the input window size (-W).  The default is 256K slots.

# Compression level #

The compression level (-0, -1, -3, -6, -9) determines the size of two internal data structures.  -9 takes about four times as much memory as -1.

# Decoder memory requirements #

To decode a VCDIFF input, the -B and -W flags are used similar to the encoding step.  For the source buffer, the value of -B or the source file size is used, whichever is smaller.  Setting -B smaller than the value used for encode will result in seeking (an LRU cache of blocks is used).

An input buffer is sized according to -W, and in addition to this, the decoder will allocate three buffers for the data, address, and instruction sections of a VCDIFF window.  The size of these buffers depends on the compressed size of a window, meaning their size depends on the value of -W used by the encoder.  If secondary compression is used, an extra set of buffers will be allocated for each secondary-compressed section.

To summarize, decoding uses -B bytes for the source buffer, plus -W bytes for its own input buffer, and three or six buffers which should total not more than one or two times the encoder's value of -W (depending on secondary compression).