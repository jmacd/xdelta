# Using Xdelta with vcpkg

This document explains how to build and use Xdelta with the [vcpkg](https://github.com/microsoft/vcpkg) package manager.

## Building with vcpkg

Xdelta supports two ways of using vcpkg:

1. **Classic mode**: Using vcpkg to install dependencies
2. **Manifest mode**: Using vcpkg.json to declare dependencies

### Prerequisites

- [vcpkg](https://github.com/microsoft/vcpkg) installed
- CMake 3.14 or newer
- A C/C++ compiler (Visual Studio, GCC, Clang)

### Classic Mode

In classic mode, you first install the dependencies using vcpkg, then build Xdelta:

```bash
# Install liblzma using vcpkg
vcpkg install liblzma

# Configure and build Xdelta
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake -DXDELTA_ENABLE_LZMA=ON
cmake --build . --config Release
```

### Manifest Mode

In manifest mode, vcpkg automatically installs the dependencies declared in the `vcpkg.json` file:

```bash
# Configure and build Xdelta
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake -DXDELTA_ENABLE_LZMA=ON
cmake --build . --config Release
```

## Build Options

- `BUILD_SHARED_LIBS`: Build shared libraries instead of static (default: OFF)
- `XDELTA_BUILD_TESTS`: Build test executables (default: OFF)
- `XDELTA_ENABLE_LZMA`: Enable LZMA compression support (default: OFF)
- `XDELTA_ENABLE_DOCS`: Build documentation (default: OFF)
- `XDELTA_USE_STATIC_RUNTIME`: Link with static C runtime on Windows (default: OFF)

## Static vs. Dynamic Linking

### Static Linking

To build Xdelta with static linking to all dependencies:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DBUILD_SHARED_LIBS=OFF -DXDELTA_USE_STATIC_RUNTIME=ON
```

### Dynamic Linking

To build Xdelta with dynamic linking:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DBUILD_SHARED_LIBS=ON
```

## Using Xdelta in Other CMake Projects with vcpkg

After installing Xdelta, you can use it in other CMake projects:

```cmake
find_package(xdelta REQUIRED)
target_link_libraries(your_target PRIVATE xdelta::xdelta)
```

Make sure to use the same vcpkg toolchain file when configuring your project:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake
```
