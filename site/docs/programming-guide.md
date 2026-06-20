# Programming guide

This guide is a quick start to the Xdelta3 C programming interface. It is not a
complete reference — the comments in `xdelta3.h` and the command-line
application in `xdelta3-main.h` offer more detail.

## Simple API

Two convenience functions encode to and decode from in-memory buffers: see
`xd3_encode_memory` and `xd3_decode_memory`.

## Advanced API

There are three external structures, two of which are discussed here:

- **`xd3_stream`** plays the main role: it holds the state needed to encode or
  decode one stream of data.
- **`xd3_source`** maintains state about the optional source file against which
  differences are computed.
- **`xd3_config`** configures encoder parameters.

At a glance the interface resembles Zlib. The program puts data in, which the
`xd3_stream` consumes. After computing over the data, the stream produces output
for the application to consume, or requests more input. The stream also asks the
application to read a block of source data. That request can be handled two
ways: if an `xd3_getblk` callback is provided, the library calls it and suspends
computation until it completes; if no callback is provided, the library returns
a special code (`XD3_GETSRCBLK`) so the application can issue the request and
resume whenever it likes. In both cases the `xd3_source` struct carries the
requested block number and a place to store the result.

## Setup

Declare and initialize the `xd3_stream`:

```c
int ret;
xd3_stream stream;
xd3_config config;

xd3_init_config(&config, 0 /* flags */);
config.winsize = 32768;
ret = xd3_config_stream(&stream, &config);

if (ret != 0) { /* error */ }
```

`xd3_init_config()` fills `xd3_config` with default values. The most relevant
setting, `xd3_config.winsize`, sets the encoder window size; the encoder
allocates a buffer of this size when the program supplies input in smaller units
(unless the `XD3_FLUSH` flag is set). `xd3_config_stream()` initializes the
`xd3_stream` with the supplied configuration.

## Setting the source

The stream is ready for input now, though for encoding the source must be
supplied first. Declare and initialize the `xd3_source`:

```c
xd3_source source;
void *IO_handle = ...;

source.name     = "...";
source.size     = file_size;
source.ioh      = IO_handle;
source.blksize  = 32768;
source.curblkno = (xoff_t) -1;
source.curblk   = NULL;

ret = xd3_set_source(&stream, &source);

if (ret != 0) { /* error */ }
```

The decoder sets the source the same way, but may delay until the application
header has been received (`XD3_GOTHEADER`). An application can check whether
source data is required for decoding with `xd3_decoder_needs_source()`.

`xd3_source.blksize` is the block size used for requesting source blocks. If the
first source block (or the entire source) is already in memory, set `curblkno`
to `0` and `curblk` to that block of data.

## Input/output loop

The application provides input by calling `xd3_avail_input()`, then drives
encoding or decoding with one of:

```c
int xd3_encode_input(xd3_stream *stream);
int xd3_decode_input(xd3_stream *stream);
```

Barring an error, these return one of six result codes the application must
handle. Much of the handling is shared between encoding and decoding:

- **`XD3_INPUT`** — the stream is ready for (or requires) more input. Call
  `xd3_avail_input` when more data is available.
- **`XD3_OUTPUT`** — the stream has pending output. Consume the block in
  `next_out` / `avail_out`, then call `xd3_consume_output`.
- **`XD3_GETSRCBLK`** — the stream is requesting a source block (only returned
  when no `xd3_getblk` callback was provided).
- **`XD3_GOTHEADER`** — decoder-specific: the first VCDIFF window header has
  been received, a chance to inspect the application header.
- **`XD3_WINSTART`** — returned by encoder and decoder before processing a
  window. For encoding, once there is enough input; for decoding, after each
  window header (except the first, which yields `XD3_GOTHEADER`).
- **`XD3_WINFINISH`** — the output for a single window has been fully consumed.

An application can be structured like this:

```c
do {
  read(&indata, &insize);
  if (reached_EOF) {
    xd3_set_flags(&stream, XD3_FLUSH);
  }
  xd3_avail_input(&stream, indata, insize);
process:
  ret = xd3_xxcode_input(&stream);   /* encode or decode */
  switch (ret) {
  case XD3_INPUT:
    continue;
  case XD3_OUTPUT:
    /* write data */
    goto process;
  case XD3_GETSRCBLK:
    /* set source block */
    goto process;
  case XD3_GOTHEADER:
  case XD3_WINSTART:
  case XD3_WINFINISH:
    /* no action necessary */
    goto process;
  default:
    /* error */
  }
} while (! reached_EOF);
```

Finally, close and free the stream. `xd3_close_stream()` checks several error
conditions but performs no I/O; `xd3_free_stream()` frees all memory the stream
allocated.

## Application header

Two routines get and set the application header. When encoding, set it before
the first `XD3_WINSTART`. When decoding, it is available after the first
`XD3_GOTHEADER`.
