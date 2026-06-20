Xdelta 3.x readme.txt
Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
2009, 2010, 2011, 2012, 2013, 2014, 2015
<josh.macdonald@gmail.com>


Thanks for downloading Xdelta!

This directory contains the Xdelta3 command-line interface (CLI) and source
distribution for VCDIFF differential compression, a.k.a. delta
compression. The latest information and downloads are available here:

  http://xdelta.org/
  http://github.com/jmacd/xdelta/

Xdelta can be configured to use XZ Utils for secondary compression:

  http://tukaani.org/xz/

The command-line syntax is detailed here:

  https://github.com/jmacd/xdelta/blob/wiki/CommandLineSyntax.md

Run 'xdelta3 -h' for brief help.  Run 'xdelta3 test' for built-in tests.

Building
--------

The recommended build uses CMake (>= 3.13) and a C99 / C++11 compiler.
liblzma (XZ Utils) is used for secondary compression when available:

  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  ctest --test-dir build          # runs the built-in regression test

This produces the `xdelta3` command-line tool (in `build/`) plus the
`xdelta3decode`, `xdelta3regtest`, and `xdelta3checksum` helpers.

liblzma is autodetected.  Force it on or off with `-DXD3_LZMA_MODE=on`
or `-DXD3_LZMA_MODE=off`.  On Homebrew systems, point CMake at the
prefix with `-DCMAKE_PREFIX_PATH="$(brew --prefix)"`.

Alternatively, build a self-contained tool by fetching and statically
linking liblzma (xz) instead of using a system copy:

  cmake -B build -DCMAKE_BUILD_TYPE=Release -DXD3_LZMA_FETCH=ON

`-DXD3_LZMA_FETCH=ON` overrides `XD3_LZMA_MODE`, requires CMake >= 3.20,
and pins the xz release with `-DXD3_LZMA_TAG=...`.  This is how the
release binaries are built, so they need no liblzma at runtime.

Armor mode (whole-file verification)
------------------------------------

By default the `xdelta3` tool builds with armor mode, which embeds BLAKE3
digests of the source and target files in the VCDIFF application header
(using a `name#blake3` syntax) and verifies them:

  * Encode reads the source and target in full to compute their digests and
    therefore requires both to be seekable (regular) files.
  * Decode verifies the source up front, before applying, and verifies the
    reconstructed target afterward.  A wrong source fails fast with a clear
    message instead of a late, ambiguous window-checksum error.
  * If the supplied source already matches the delta's *target* digest (the
    patch is already applied), decode reports "already up to date" and exits
    with status 2.
  * If the source is a stream (not seekable) it cannot be verified, so decode
    prints a warning and proceeds.
  * The digests cover the logical (decompressed) content xdelta3 processes,
    not the raw on-disk bytes, so armor composes correctly with external
    input decompression and output recompression.
  * `merge` verifies an armored chain of deltas when every input is armored.

Pass `-a` to disable armor: this restores the legacy application-header
format and the non-seekable streaming behavior (e.g. for piped input).
Legacy xdelta3 builds read the armored names as literal filenames, so they
must apply such deltas with explicit `-s`/output filenames.

BLAKE3 is fetched at configure time from its official repository, pinned to
a release tag (`-DXD3_BLAKE3_TAG=...`).  Disable armor entirely at build time
with `-DXD3_ARMOR=OFF`, which removes the BLAKE3 dependency and makes the
tool behave as if `-a` were always given.

Testing
-------

`xdelta3 test` and `ctest` run the built-in C test suite.  An additional
regression test harness, written in Go (1.21+), drives the command-line
tool end to end:

  (cd go && go run . -xdelta3 ../build/xdelta3)

See `go run . -h` in the `go/` directory for its flags (including the
optional `-dataset`/`-compare` comparison test).


Formatting
----------

C/C++ sources are formatted with clang-format (version 20) using the
LLVM style in `.clang-format`.  The Objective-C iOS example is excluded.

  ./format.sh            # reformat sources in place
  ./format.sh --check    # verify formatting (used by CI)

Set `CLANG_FORMAT` to select a specific clang-format binary.


Sample commands (like gzip, -e means encode, -d means decode)

  xdelta3 -9 -S lzma -e -f -s OLD_FILE NEW_FILE DELTA_FILE
  xdelta3 -d -s OLD_FILE DELTA_FILE DECODED_FILE

File bug reports and browse open support issues here:

  https://github.com/jmacd/xdelta/issues

The source distribution contains the C/C++ library and command-line
tool, built with CMake (see "Building" above).  Xdelta3 is covered under
the terms of the APL, see LICENSE.
