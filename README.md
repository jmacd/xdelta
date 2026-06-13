# Xdelta

[![CI](https://github.com/jmacd/xdelta/actions/workflows/ci.yml/badge.svg)](https://github.com/jmacd/xdelta/actions/workflows/ci.yml)

Xdelta version 3 is a C library and command-line tool for delta
compression using VCDIFF/RFC 3284 streams.

# Building

The sources live in the [`xdelta3`](xdelta3) directory.  To build with
CMake:

```sh
cd xdelta3
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

See [`xdelta3/README.md`](xdelta3/README.md) for more detail, including
liblzma options and the Go regression-test harness.

# License

This repository contains branches of Xdelta 3.x that were
re-licensed by the original author under the [Apache Public
License version 2.0](http://www.apache.org/licenses/LICENSE-2.0),
namely:

- __release3_0_apl__ Change to APL based on 3.0.11 sources
- __release3_1_apl__ Merges release3_0_apl with 3.1.0 sources

The original GPL licensed Xdelta lives at http://github.com/jmacd/xdelta-gpl.

# Documentation

See the [command-line usage](https://github.com/jmacd/xdelta/blob/wiki/CommandLineSyntax.md).  See [wiki directory](https://github.com/jmacd/xdelta/tree/wiki).




