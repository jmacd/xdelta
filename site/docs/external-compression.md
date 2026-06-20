# External compression

To compute a delta between *externally-compressed* inputs (for example
`xdelta3 -e -s source.gz target.gz`), xdelta3 recognizes several common
compression programs and decompresses the data before reading it, including
`gzip`, `bzip2`, and `compress`. When the decoder finishes, it re-applies the
external compression to the output.

!!! warning "Not available on Windows"
    External compression is not supported on Windows. Decompress the inputs
    manually before running xdelta3.

## How it works

Xdelta decompresses the inputs by piping them through the external compression
program. Recognition of externally-compressed inputs can be disabled with `-D`.

External compression has a well-known pitfall: xdelta3 does not know the
original compression settings, so when re-applying the external compression it
uses default settings, which may produce different compressed bytes and break
checksum verification of the compressed data. External recompression of the
output can be disabled with `-R`. If you know the settings needed to reproduce
the exact output (for example `gzip -9`), set the corresponding environment
variable to control the external command (for example `export GZIP=-9`).

```sh
gzip release-1.tar
gzip release-2.tar
xdelta3 -e -s release-1.tar.gz release-2.tar.gz delta-1-2.xd3
xdelta3 -d -s release-1.tar.gz delta-1-2.xd3 release-2.tar.gz
```

Because of these pitfalls, xdelta3 prints a warning when it auto-decompresses:

```
xdelta3: WARNING: the encoder is automatically decompressing the input file;
xdelta3: WARNING: the decoder will automatically recompress the output file;
xdelta3: WARNING: this may result in different compressed data and checksums
xdelta3: WARNING: despite being identical data; if this is an issue, use -D
xdelta3: WARNING: to avoid decompression and/or use -R to avoid recompression
xdelta3: WARNING: and/or manually decompress the input file; if you know the
xdelta3: WARNING: compression settings that will produce identical output
xdelta3: WARNING: you may set those flags using the environment (e.g., GZIP=-9)
```

!!! note "Interaction with armor mode"
    [Armor mode](armor.md) digests cover the logical (decompressed) content
    xdelta3 processes, not the raw on-disk bytes, so whole-file verification
    composes correctly with external decompression of inputs and recompression
    of output.
