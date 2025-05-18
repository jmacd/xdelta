# Xdelta Utility Scripts

This directory contains utility scripts for the Xdelta project.

## Format String Fix Scripts

### fix_format_strings.ps1

This PowerShell script automatically fixes C++11 literal suffix format string issues in all C/C++ files in the repository. These issues can cause compilation errors on some platforms, particularly macOS with Clang.

The script searches for patterns like `%"Q"u` and replaces them with the correct format `%" Q "u` (adding spaces between the quotes and letters).

**Usage:**
```powershell
powershell -ExecutionPolicy Bypass -File scripts\fix_format_strings.ps1
```

### fix_xdelta3_format_strings.ps1

This script specifically targets the xdelta3.c file, which may contain many format string issues. It applies a more focused set of replacements to this file.

**Usage:**
```powershell
powershell -ExecutionPolicy Bypass -File scripts\fix_xdelta3_format_strings.ps1
```

## Background

C++11 introduced stricter rules for string literals with user-defined suffixes. Format strings like `%"Q"u` need to have spaces between the quotes and letters to avoid being interpreted as user-defined literals, which would cause compilation errors.

The correct format is `%" Q "u` (with spaces between quotes and letters).

This issue primarily affects macOS builds with Clang, which enforces these rules more strictly than other compilers.

## Alternative Solutions

If you prefer not to modify the source code, you can also use compiler flags to suppress these warnings/errors:

- For Clang: `-Wno-reserved-user-defined-literal` or `-Wno-error=reserved-user-defined-literal`
- For GCC: `-Wno-literal-suffix`

You can add these to your CMakeLists.txt file:

```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wno-reserved-user-defined-literal)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options(-Wno-literal-suffix)
endif()
```
