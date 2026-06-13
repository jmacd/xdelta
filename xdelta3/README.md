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

Testing
-------

`xdelta3 test` and `ctest` run the built-in C test suite.  An additional
regression test harness, written in Go (1.21+), drives the command-line
tool end to end:

  (cd go && go run . -xdelta3 ../build/xdelta3)

See `go run . -h` in the `go/` directory for its flags (including the
optional `-dataset`/`-compare` comparison test).


Sample commands (like gzip, -e means encode, -d means decode)

  xdelta3 -9 -S lzma -e -f -s OLD_FILE NEW_FILE DELTA_FILE
  xdelta3 -d -s OLD_FILE DELTA_FILE DECODED_FILE

File bug reports and browse open support issues here:

  https://github.com/jmacd/xdelta/issues

The source distribution contains the C/C++ library and command-line
tool, built with CMake (see "Building" above).  Xdelta3 is covered under
the terms of the APL, see LICENSE.
