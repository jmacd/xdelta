# Building Xdelta 1.1.5

Xdelta 1.1.5 uses CMake and requires a C compiler, CMake 3.20 or later,
pkg-config, GLib 2, and zlib.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build
```

The historical Autotools files remain for preservation, but require GLib 1
and are not supported for the 1.1.5 release.
