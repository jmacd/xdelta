# Xdelta CMake Build System

This directory contains CMake configuration files for building Xdelta.

## Building with CMake

### Basic Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Configuration Options

The following options can be used to customize the build:

- `BUILD_SHARED_LIBS`: Build shared libraries instead of static (default: OFF)
- `XDELTA_BUILD_TESTS`: Build test executables (default: OFF)
- `XDELTA_ENABLE_LZMA`: Enable LZMA compression support (default: ON)
- `XDELTA_ENABLE_DOCS`: Build documentation (default: OFF)

Example:

```bash
cmake .. -DBUILD_SHARED_LIBS=ON -DXDELTA_BUILD_TESTS=ON
```

### Installing

```bash
cmake --build . --target install
```

By default, this will install to system directories. To install to a custom location:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
cmake --build . --target install
```

## Using Xdelta in Other CMake Projects

After installing Xdelta, you can use it in other CMake projects:

```cmake
find_package(xdelta REQUIRED)
target_link_libraries(your_target PRIVATE xdelta::xdelta)
```
