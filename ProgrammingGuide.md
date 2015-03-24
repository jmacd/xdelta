# Introduction #

This guide intends to give you a quick start to the Xdelta3 programming
interface.  This is not a complete reference, the comments inside source file
`xdelta3.h` and the command-line application,
`xdelta3-main.h` offer more complete information.

## Simple API ##

There are two convenience functions for encoding to and decoding from
in-memory buffers.  See the `xd3_encode_memory` and
`xd3_decode_memory` interfaces.


## Advanced API ##

There are three external structures, only two of which are
discussed here.  The `xd3_stream` struct plays the main role, one
of these contains the state necessary to encode or decode one stream of data.
An `xd3_source` struct maintains state about the (optional) source
file, against which differences are computed.  The third structure,
`xd3_config` deals with configuring various encoder parameters.


At a glance, the interface resembles Zlib.  The program puts data in, which
the xd3\_stream consumes.  After computing over the data, the xd3\_stream in
turn generates output for the application to consume, or it requests more
input.  The xd3\_stream also issues requests to the application to read a block
of source data.  The request to read a source block may be handled in one of
two ways, according to application preference.  If a `xd3_getblk`
callback function is provided, the application handler will be called from
within the library, suspending computation until the request completes.  If no
callback function is provided the library returns a special code
(XD3\_GETSRCBLK), allowing the application to issue the request and resume
computation whenever it likes.  In both cases, the xd3\_source struct contains
the requested block number and a place to store the result.

## Setup ##
The code to declare and initialize the xd3\_stream:

```
int ret;
xd3_stream stream;
xd3_config config;

xd3_init_config (&config, 0 /* flags */);
config.winsize = 32768;
ret = xd3_config_stream (&stream, &config);

if (ret != 0) { /* error */ }
```


`xd3_init_config()` initializes the `xd3_config` struct
with default values.  Many settings remain undocumented in the beta release.
The most relevant setting, `xd3_config.winsize`, sets the encoder
window size.  The encoder allocates a buffer of this size if the program
supplies input in smaller units (unless the `XD3_FLUSH` flag is
set). `xd3_config_stream()` initializes the `xd3_stream`
object with the supplied configuration.


## Setting the source ##

The stream is ready for input at this point, though for encoding the source
data must be supplied now.  To declare an initialize the xd3\_source:

```
xd3_source source;
void *IO_handle = ...;

source.name = "...";
source.size = file_size;
source.ioh= IO_handle;
source.blksize= 32768;
source.curblkno = (xoff_t) -1;
source.curblk = NULL;

ret = xd3_set_source (&stream, &source);

if (ret != 0) { /* error */ }
```

The decoder sets source data in the same manner, but it may delay this step
until the application header has been received (`XD3_GOTHEADER`).
The application can also check whether source data is required for decoding
with the `xd3_decoder_needs_source()`.


`xd3_source.blksize` determines the block size used for requesting
source blocks.  If the first source block (or the entire source) is already in
memory, set `curblkno` to 0 and `curblk` to that block
of data.

## Input/output loop ##

The stream is now ready for input, which the application provides by
calling `xd3_avail_input()`.  The application initiates
encoding or decoding at this point by calling one of two functions:

```
int xd3_encode_input (xd3_stream *stream)
int xd3_decode_input (xd3_stream *stream)
```

Unless there is an error, these routines return one of six result
codes which the application must handle.  In many cases, all or most
of the handler code is shared between encoding and decoding.  The
codes are:

  * `XD3_INPUT`: The stream is ready for (or requires) more input.  The application should call xd3\_avail\_input when (if) more data is available.

  * `XD3_OUTPUT`: The stream has pending output.  The application should write or otherwise consume the block of data found in the xd3\_stream fields `next_out` and `avail_out`,then call `xd3_consume_output`.

  * `XD3_GETSRCBLK`: The stream is requesting a source block be read,as described above.  This is only returned when the xd3\_getblk callback was not provided.

  * `XD3_GOTHEADER`: This decoder-specific code indicates that the first VCDIFF window header has been received.  This gives the application a chance to inspect the application header before encoding the first window.

  * `XD3_WINSTART`: This is returned by both encoder and decoder prior to processing a window.  For encoding, this code is returned once there is enough available input.  For decoding, this is returned following each window header (except the first, when XD3\_GOTHEADER is returned instead).

  * `XD3_WINFINISH`: This is called when the output from a single window has been fully consumed.

An application could be structured something like this:

```
do {
  read (&indata, &insize);
  if (reached_EOF) {
    xd3_set_flags (&stream, XD3_FLUSH);
  }
  xd3_avail_input (&stream, indata, insize);
process:
  ret = xd3_xxcode_input (&stream);
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


All that remains is to close the stream and free its resources.  The
`xd3_close_stream()` checks several error conditions but otherwise
involves no input or output.  The `xd3_free_stream()` routine frees
all memory allocated by the stream.

## Misc ##


There are two routines to get and set the application header.  When
encoding, the application header must be set before the first
`XD3_WINSTART`.  When decoding, the application header is available
after after the first `XD3_GOTHEADER`.