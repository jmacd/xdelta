# Introduction #

To compute a delta between _externally-compressed_ inputs (e.g., `xdelta3 -e -s source.gz target.gz`), xdelta3 recognizes several common compression programs and uncompresses the data before reading it, including `gzip`, `bzip2`, and `compress`. When the decoder finishes decompression, it re-applies the external compression.

External compression is not supported on Windows.  Windows users should manually decompress the inputs prior to running xdelta3 (sorry).

Currently [issue 132](https://code.google.com/p/xdelta/issues/detail?id=132) is affecting external decompression; it may cause the process not to terminate.

# Details #

Xdelta decompresses the inputs using pipes to the external compression program.

Recognition of externally-compressed inputs can be disabled by `-D`.

External compression has a well known problem: xdelta3 does not know the original compression settings, so it when re-applying the external compression it uses defaul settings, which may break checksum verification of the compressed data. External recompression (of the output) can be disabled by `-R`.  If you know the compression settings needed for the external compression to produce the exact output (e.g., gzip -9), consider setting an environment variable to control the external compression command (e.g., export GZIP=-9).

Example:

```
gzip release-1.tar
gzip release-2.tar
xdelta3 -e -s release-1.tar.gz release-2.tar.gz delta-1-2.xd3
xdelta3 -d -s release-1.tar.gz delta-1-2.xd3 release-2.tar.gz
```

Because of the above-mentioned pitfalls, xdelta3 prints the following warning:

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