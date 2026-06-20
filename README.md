# Xdelta

[![CI](https://github.com/jmacd/xdelta/actions/workflows/ci.yml/badge.svg)](https://github.com/jmacd/xdelta/actions/workflows/ci.yml)

Xdelta version 3 is a C library and command-line tool for delta
compression using VCDIFF/RFC 3284 streams.

The current release series is **3.2.x**.

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

# Releases

Prebuilt binaries for Linux, macOS, and Windows, plus a source tarball, are
attached to each [GitHub Release](https://github.com/jmacd/xdelta/releases).
The binaries are self-contained: liblzma (xz) and BLAKE3 are statically
linked, so they need no system libraries at runtime.  Releases are cut by
pushing a `vMAJOR.MINOR.PATCH` tag, which drives the
[release workflow](.github/workflows/release.yml).  See
[`RELEASING.md`](RELEASING.md) for the full procedure and the release-branch
conventions (`releaseMAJOR_MINOR_apl`).

# License

This repository contains branches of Xdelta 3.x that were
re-licensed by the original author under the [Apache Public
License version 2.0](http://www.apache.org/licenses/LICENSE-2.0),
namely:

- __release3_0_apl__ Change to APL based on 3.0.11 sources
- __release3_1_apl__ Merges release3_0_apl with 3.1.0 sources

The `main` branch and the 3.2.x series (`release3_2_apl`) continue under the
same Apache 2.0 license.

The original GPL licensed Xdelta lives at http://github.com/jmacd/xdelta-gpl.

# Documentation

Full documentation is published at **<https://jmacd.github.io/xdelta/>**,
including the [command-line syntax](https://jmacd.github.io/xdelta/commandline/),
[armor mode](https://jmacd.github.io/xdelta/armor/), and the
[programming guide](https://jmacd.github.io/xdelta/programming-guide/).

The site sources live in [`site/`](site) (MkDocs + Material) and are built and
deployed by [`.github/workflows/site.yml`](.github/workflows/site.yml).  These
pages were migrated and updated from the project's legacy
[`wiki` branch](https://github.com/jmacd/xdelta/tree/wiki).




