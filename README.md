# Xdelta

Xdelta version 3 is a C library and command-line tool for delta
compression using VCDIFF/RFC 3284 streams.

# License

This repository contains branches of Xdelta 3.x that were
re-licensed by the original author under the [Apache Public
License version 2.0](http://www.apache.org/licenses/LICENSE-2.0),
namely:

- __release3_0_apl__ Change to APL based on 3.0.11 sources
- __release3_1_apl__ Merges release3_0_apl with 3.1.0 sources

The original GPL licensed Xdelta lives at http://github.com/jmacd/xdelta-gpl.

# Build Requirements

## Compiler Requirements

Xdelta3 requires a C11-compatible compiler. The following minimum compiler versions are supported:

- **GCC**: 4.9 or later
- **Clang**: 3.4 or later
- **MSVC**: Visual Studio 2015 (14.0) or later

## C/C++ Standard Requirements

- **C++ Standard**: C++14 (ISO/IEC 14882:2014)
- **C Standard**: C11 (ISO/IEC 9899:2011) when compiled as C

## Dependencies

- **LZMA**: Required for LZMA compression support
- **CMake**: 3.15 or later for build system

## Platform-Specific Notes

### Windows

- Use Visual Studio 2015 or later
- vcpkg is recommended for managing dependencies
- Set the following CMake options:
  ```
  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
  ```

### macOS

- Use Clang from Xcode 6.0 or later
- Ensure C11 standard is enabled with `-std=c11` flag
- Install dependencies via Homebrew:
  ```
  brew install cmake liblzma
  ```

### Linux

- GCC 4.9+ or Clang 3.4+ is required
- Ensure C11 standard is enabled with `-std=c11` flag
- Install dependencies:
  ```
  # Debian/Ubuntu
  apt-get install cmake liblzma-dev

  # Fedora/RHEL
  dnf install cmake xz-devel
  ```

## Compilation Flags

For all platforms, ensure the following compilation flags are set:

- `-std=c++14` (GCC/Clang) or `/std:c++14` (MSVC)
- `-D_FILE_OFFSET_BITS=64` for large file support

## Known Issues

- The code is now compiled as C++ using C++14 standard for better cross-platform compatibility
- Some compiler warnings about unused parameters or type conversions may appear but can be safely ignored
- The original C code is maintained but compiled with a C++ compiler to avoid platform-specific C11 compatibility issues

# Documentation

See the [command-line usage](https://github.com/jmacd/xdelta/blob/wiki/CommandLineSyntax.md).  See [wiki directory](https://github.com/jmacd/xdelta/tree/wiki).


