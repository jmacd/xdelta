# Building Xdelta 1.2.0

Xdelta 1.2.0 uses CMake and requires a C compiler, CMake 3.20 or later,
pkg-config, GLib 2, and zlib.

```

The checked-in serialization sources are generated from `xd.ser` and
`libedsio/edsio.ser`. Regenerate them after changing either schema or
`libedsio/edsio.el`:

```sh
./generate-edsio.sh
```

Set `EMACS` to select a non-default Emacs executable.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build
```

Autotools is not supported. CMake is the only supported build system.
