# Better compression (secondary compression)

[VCDIFF](https://www.rfc-editor.org/rfc/rfc3284.txt) specifies a byte-code data
format that does not use variable-bit-length codes. A byte-code-only format that
is good on its own is desirable for many reasons, but a variable-bit-length
encoding is naturally more efficient than a fixed-length code. VCDIFF also
includes options for encoding and decoding with alternate byte-code tables.

## Compression levels

The `-1` … `-9` flags select alternate compression levels: `-1` is fastest and
`-9` gives the best string-matching compression (at higher memory cost; see
[Tuning the memory budget](tuning-memory.md)).

## Secondary compression

Even better compression can be achieved by applying a *secondary* compressor to
the VCDIFF byte stream, selected with `-S`:

- **`-S lzma`** — [LZMA](https://tukaani.org/xz/) (XZ Utils). Available when
  xdelta3 was built with liblzma; usually the strongest option.
- **`-S djw`** — the built-in DJW coder (described below).
- **`-S fgk`** — the built-in adaptive Huffman (FGK) coder.
- **`-S none`** — disable secondary compression.

## The DJW coder

DJW is named after [David Wheeler](https://en.wikipedia.org/wiki/David_Wheeler_(computer_scientist)).
It is a semi-adaptive Huffman code in the spirit of the entropy-coding stage
that follows the [Burrows–Wheeler transform](https://en.wikipedia.org/wiki/Burrows%E2%80%93Wheeler_transform)
in [bzip2](https://en.wikipedia.org/wiki/Bzip2).

The algorithm works with multiple Huffman-code tables over several iterations.
The input is divided into fixed-length chunks, each chunk assigned to a Huffman
table. On each iteration, chunks are reassigned to the table that gives the
shortest encoding, then the tables are recomputed from the chunks assigned to
them.

The chunk encodings plus the chunk-to-table assignments are transformed by
[move-to-front](https://en.wikipedia.org/wiki/Move-to-front_transform).
Move-to-front works well (especially following Burrows–Wheeler), but it presents
a problem for later Huffman coding: afterward there tend to be many `0`s, and a
symbol with frequency greater than 50% cannot be encoded efficiently by Huffman
coding, since even the shortest 1-bit code has redundancy. To address this, the
`0` symbol is replaced by two symbols (call them `0_0` and `0_1`) that encode
the run length of `0`s in binary.

Xdelta3 also implements alternate code tables, but little work has been done on
their optimization potential. See the `GENERIC_ENCODE_TABLES=1` build flag and
related tests, which are not compiled by default.

## See also

[bsdiff](https://www.daemonology.net/bsdiff/) is another differential
compression program, based on bzip2.
