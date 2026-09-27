# Xdelta3 API examples

These programs demonstrate specific parts of the Xdelta3 API.

- `small_page_test.c` demonstrates using Xdelta3 with little memory, for
  example on kernel-sized pages.
- `encode_decode_test.c` demonstrates the state machine of the non-blocking
  `xd3_encode_input` and `xd3_decode_input` API. It streams the target and
  delta, services `XD3_GETSRCBLK` requests itself, and prints every state
  transition.
- `speed_test.c` is an encoder microbenchmark for the `xd3_encode_memory`
  convenience API. It reads both files completely into memory before timing
  begins and does not benchmark decoding.
- `compare_test.c` benchmarks internal match-finding routines. It is not an
  example of the public API.

Copyrights are held by the respective authors.

## `encode_decode_test`

Run the non-blocking encode/decode example with:

```console
./encode_decode_test OLD_FILE NEW_FILE
```

It writes the delta to `encoded.testdata`, decodes that delta, and writes the
result to `decoded.testdata`. The program prints each state returned by
`xd3_encode_input` and `xd3_decode_input`, including source-block requests and
window boundaries.

The example uses a 4 KiB buffer for target windows and source-block reads.
This keeps its memory use small and makes state transitions easy to observe,
but causes frequent source-file seeks and reads. Applications should choose
`config.winsize` and `source.blksize` for their workloads; larger powers of two
generally reduce I/O and window overhead at the cost of memory. The
[programming guide][guide] documents the non-blocking state machine.

## `speed_test`

Run the in-memory encoder benchmark with:

```console
./speed_test64 LEVEL COUNT OLD_FILE NEW_FILE
```

`LEVEL` selects compression level 0 through 9 and `COUNT` selects the number of
timed encoding iterations. `speed_test` reads both files and allocates its
output buffer before timing begins. Each iteration calls `xd3_encode_memory`;
the benchmark does not include file input, allocation, output writing, or
decoding.

Both input files must fit in memory. `xd3_encode_memory` makes the entire source
available as one block and uses target windows up to `XD3_DEFAULT_WINSIZE`
(currently 8 MiB). The output buffer is allocated at 110 percent of the target
size, so inputs whose delta exceeds that capacity cause the example to fail
with `ENOSPC`.

The `speed_test32` and `speed_test64` targets select Xdelta3's 32-bit and
64-bit file-offset configurations; they do not limit process memory.

## Building

The example Makefile compiles selected programs by including `xdelta3.c`
directly. Its default `CFLAGS` intentionally enable debugging and disable
optimization:

```sh
make
```

For representative timing, select the optimized `CFLAGS` line in the
Makefile or override it explicitly with the platform configuration macros
required by your build. Applications should normally link against the CMake
library target described in the [library documentation][library] rather than
include `xdelta3.c`.

[guide]: ../../site/docs/programming-guide.md
[library]: ../README.md#using-xdelta3-as-a-library
