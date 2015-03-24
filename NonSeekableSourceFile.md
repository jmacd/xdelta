How to compress when both the source and the target are streams, named pipes, etc.

# Introduction #

It is possible to read the **source file** from a FIFO or a named pipe. The **encoder** is designed not to seek backward, but this has implications for the **decoder** memory budget.

## Details ##

The source window size (set by the -B flag) used for the decoder must be **greater than or equal to** the source window size used for the encoder.

This mode is the default for ExternalCompression, if you pass a compressed source file at the command-line interface, xdelta3 forks the decompression command and reads the source file through a pipe.

You will see

```
xdelta3: non-seekable source: copy is too far back (try raising -B): XD3_INTERNAL
```

Xdelta _should_ transmit this information.  See [issue 117](https://code.google.com/p/xdelta/issues/detail?id=117).