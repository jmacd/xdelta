# Introduction #

[VCDIFF](http://www.rfc-editor.org/rfc/rfc3284.txt) specifies a byte-code data format. The data format does not use variable-bit-length codes.  This was done because a byte-code-only format that is good on its own is desireable for many reasons, but a variable-bit-length encoding is naturally more efficient than a fixed-length code. VCDIFF also includes options for encoding/decoding with alternate byte-code tables.

# Details #

The flags `-1`, ... `-9` flags support alternate compression levels: faster/better compression options. If you want fastest compression, try `-1`. If you want best compression, try `-9`. Even better compression can be achieved using a variable-bit-length encoding, i.e., secondary compression.  Xdelta3 supports a method called DJW, enabled by the command-line flag `-S djw`.

Named after [Wheeler](http://www.thocp.net/biographies/wheeler_david.htm). DJW described this encoding to follow the [Burrows-Wheeler](http://en.wikipedia.org/wiki/Burrows-Wheeler_transform) transform, a sort of semi-adaptive Huffman code implemented in [bzip2](http://en.wikipedia.org/wiki/Bzip2) (following the Burrows-Wheeler transform). DJW's [decription](ftp://ftp.cl.cam.ac.uk/users/djw3/bred3.ps), DJW's [code](ftp://ftp.cl.cam.ac.uk/users/djw3/bred3.c). The algorithm works with multiple Huffman-code tables and several iterations. The input is divided into fixed-length chunks, each chunk assigned to a Huffman table. Each iteration, chunks are assigned to the table which gives the shortest encoding, then the tables are recomputed according to the chunks they were assigned.

The chunk encodings plus the chunk-table assignments are then transformed by [move-to-front](http://en.wikipedia.org/wiki/Move-to-front_transform). Move-to-front works very well (especially following Burrows-Wheeler), but it presents another problem for later Huffman coding, because after move-to-front coding there tend to be many `0`s, and a symbol having frequency greater than 50% will not be efficiently encoded by Huffman coding--even the shortest 1-bit code has redundency. To address this problem, the `0` symbol is replaced by two symbols (call them `0_0` and `0_1`).  These two symbols are used to code the run-length of `0`s in binary.

Xdelta3 implements alternate code tables, but no work has been done on their optimization potential.  See the -DGENERIC\_ENCODE\_TABLES=1 flag and related tests, which are not compiled by default.

See also: [bsdiff](http://www.daemonology.net/bsdiff/) is another differential compression program, based on bzip2.