# Xdelta vcpkg Registry

This is a vcpkg registry for the Xdelta library and tools, allowing users to easily install pre-built xdelta binaries via vcpkg.

## Usage

To use this registry, create a `vcpkg-configuration.json` file in your project root with the following content:

```json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/loonghao/xdelta",
      "baseline": "latest commit SHA of the main branch",
      "packages": [ "xdelta" ]
    }
  ]
}
```

Then you can install xdelta using vcpkg:

```
vcpkg install xdelta
```

## Available Packages

- **xdelta** - Xdelta binary diff and differential compression tools (version 3.1.0)

## Using in CMake Projects

In your CMake project, you can use xdelta like this:

```cmake
find_package(xdelta CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE xdelta::xdelta)
```

Or directly use the library files:

```cmake
target_include_directories(your_target PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
target_link_libraries(your_target PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/xdelta.lib)
```

## Using the Command Line Tool

After installation, you can find the xdelta3 command line tool at:

```
${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/xdelta/xdelta3.exe
```

## License

The registry itself is licensed under MIT.
Xdelta is licensed under the Apache License 2.0.
