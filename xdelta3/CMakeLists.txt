# CMake build for xdelta3.
#
# This is the cross-platform build.
#
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   ctest --test-dir build        # runs the built-in regression test
#
cmake_minimum_required(VERSION 3.13)

project(xdelta3
  VERSION 3.2.0
  DESCRIPTION "VCDIFF (RFC 3284) delta compression library and tool"
  HOMEPAGE_URL "https://github.com/jmacd/xdelta"
  LANGUAGES C CXX)

option(XD3_BUILD_TESTS "Build the regression-test executables" ON)

set(XD3_LZMA_MODE "auto" CACHE STRING
  "liblzma secondary compression: auto, on, or off")
set_property(CACHE XD3_LZMA_MODE PROPERTY STRINGS auto on off)

# When ON, liblzma (xz) is fetched and built as a static library via
# FetchContent (pinned to XD3_LZMA_TAG) and linked into xdelta3, instead of
# using a system liblzma.  This yields a dependency-free binary with liblzma
# secondary compression and is what the release workflow uses.  It overrides
# XD3_LZMA_MODE and requires CMake >= 3.20 (the minimum xz's build needs).
option(XD3_LZMA_FETCH "Fetch and statically link liblzma (xz) via FetchContent" OFF)
set(XD3_LZMA_TAG "v5.8.3" CACHE STRING
  "xz/liblzma release tag to fetch when XD3_LZMA_FETCH=ON")

option(XD3_WERROR "Treat compiler warnings as errors for the core targets" OFF)

# Window size type width.  ON (default) selects a 64-bit usize_t; OFF selects
# the historical/embedded 32-bit usize_t (XD3_USE_LARGESIZET=0) where absolute
# 64-bit xoff_t offsets are narrowed into 32-bit window-relative usize_t
# values.  The OFF build exercises those narrowing paths so the regression
# suite covers the mixed-width configuration.
option(XD3_LARGESIZET "Use a 64-bit window size type (usize_t)" ON)

# Armor mode: whole-file BLAKE3 verification carried in the VCDIFF app-header.
# When ON, the official BLAKE3 C library is fetched (pinned tag) and linked into
# the xdelta3 command-line tool.  When OFF, armor is compiled out and the tool
# behaves as if -a (disable armor) were always given.
option(XD3_ARMOR "Enable armor mode (BLAKE3 whole-file verification)" ON)
set(XD3_BLAKE3_TAG "1.8.5" CACHE STRING
  "BLAKE3 release tag to fetch for armor mode")

include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckCSourceRuns)

# ---------------------------------------------------------------------------
# Feature / size detection -> config.h
# ---------------------------------------------------------------------------
check_type_size("size_t"             SIZEOF_SIZE_T)
check_type_size("unsigned int"       SIZEOF_UNSIGNED_INT)
check_type_size("unsigned long"      SIZEOF_UNSIGNED_LONG)
check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)

# liblzma (secondary compression).
set(HAVE_LZMA_H OFF)
set(XD3_LZMA_LIBRARIES "")
if(XD3_LZMA_FETCH)
  if(CMAKE_VERSION VERSION_LESS 3.20)
    message(FATAL_ERROR
      "XD3_LZMA_FETCH=ON requires CMake >= 3.20 (xz's build); "
      "use a system liblzma via XD3_LZMA_MODE instead.")
  endif()
  # Build a minimal static liblzma: no CLI tools, docs, or translations.
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(XZ_NLS OFF CACHE BOOL "" FORCE)
  set(XZ_DOC OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_XZ OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_XZDEC OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_LZMADEC OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_LZMAINFO OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_SCRIPTS OFF CACHE BOOL "" FORCE)
  set(XZ_TOOL_SYMLINKS OFF CACHE BOOL "" FORCE)
  include(FetchContent)
  FetchContent_Declare(liblzma
    GIT_REPOSITORY https://github.com/tukaani-project/xz.git
    GIT_TAG ${XD3_LZMA_TAG}
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(liblzma)
  # xz marks the public api/ directory PRIVATE on its in-tree target, so expose
  # it (and the LZMA_API_STATIC define it sets) to consumers ourselves.
  target_include_directories(liblzma INTERFACE
    "$<BUILD_INTERFACE:${liblzma_SOURCE_DIR}/src/liblzma/api>")
  set(HAVE_LZMA_H ON)
  set(XD3_LZMA_LIBRARIES liblzma)
  message(STATUS "xdelta3: liblzma fetched and statically linked (xz ${XD3_LZMA_TAG})")
elseif(NOT XD3_LZMA_MODE STREQUAL "off")
  find_package(LibLZMA)
  if(LibLZMA_FOUND)
    set(HAVE_LZMA_H ON)
    set(XD3_LZMA_LIBRARIES LibLZMA::LibLZMA)
  else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(LIBLZMA IMPORTED_TARGET QUIET liblzma)
      if(LIBLZMA_FOUND)
        set(HAVE_LZMA_H ON)
        set(XD3_LZMA_LIBRARIES PkgConfig::LIBLZMA)
      endif()
    endif()
  endif()
  if(NOT HAVE_LZMA_H AND XD3_LZMA_MODE STREQUAL "on")
    message(FATAL_ERROR "XD3_LZMA_MODE=on but liblzma was not found")
  endif()
endif()
message(STATUS "xdelta3: liblzma secondary compression: ${HAVE_LZMA_H}")

# Does the target require naturally-aligned memory access?
set(HAVE_ALIGNED_ACCESS_REQUIRED OFF)
if(NOT CMAKE_CROSSCOMPILING)
  check_c_source_runs("
    int main(void) {
      static char buf[8] = {0};
      unsigned int *p = (unsigned int *)(buf + 1);
      *p = 0x12345678u;
      return (*p == 0x12345678u) ? 0 : 1;
    }" XD3_UNALIGNED_ACCESS_OK)
  if(NOT XD3_UNALIGNED_ACCESS_OK)
    set(HAVE_ALIGNED_ACCESS_REQUIRED ON)
  endif()
else()
  # Cannot run a probe; assume strict alignment for safety.
  set(HAVE_ALIGNED_ACCESS_REQUIRED ON)
endif()

include(CheckCSourceRuns)

configure_file(config.h.cmake config.h @ONLY)

# ---------------------------------------------------------------------------
# Common compiler flags.
# ---------------------------------------------------------------------------
include(CheckCCompilerFlag)
set(XD3_WFLAGS "")
if(NOT MSVC)
  foreach(flag
      -Wall -Wshadow -fno-builtin -Wextra -Wsign-compare
      -Wformat=2 -Wno-format-nonliteral
      -Wno-unused-parameter -Wno-unused-function)
    list(APPEND XD3_WFLAGS ${flag})
  endforeach()
endif()

# config.h lives in the build directory; the sources are in the source dir.
set(XD3_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# Platform I/O selection and feature defines.  On Windows, use the Win32
# file API and disable features that need a POSIX shell or subprocesses
# (external compression and the shell-based self tests).  Targets that use
# the portable C stdio path (xdelta3decode) set their own XD3_STDIO=1.
set(XD3_PLATFORM_DEFS "")
if(WIN32)
  set(XD3_PLATFORM_DEFS
    XD3_WIN32=1 XD3_POSIX=0 XD3_STDIO=0
    EXTERNAL_COMPRESSION=0 SHELL_TESTS=0)
endif()

# Warnings-as-errors flags, applied only to the core targets (the C++ test
# harness has its own pre-existing warnings).  GCC is intentionally
# excluded: it emits many additional, largely false-positive diagnostics
# (implicit-fallthrough, format-truncation, stringop-overflow) that are
# out of scope here.
set(XD3_WERROR_FLAGS "")
if(XD3_WERROR)
  if(MSVC)
    set(XD3_WERROR_FLAGS /WX)
  elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(XD3_WERROR_FLAGS -Werror)
  endif()
endif()

# Helper to apply shared settings to a target.
function(xd3_configure_target target)
  target_include_directories(${target} PRIVATE ${XD3_INCLUDE_DIRS})
  target_compile_definitions(${target} PRIVATE HAVE_CONFIG_H=1
    XD3_USE_LARGESIZET=$<BOOL:${XD3_LARGESIZET}>)
  target_compile_options(${target} PRIVATE ${XD3_WFLAGS})
  set_target_properties(${target} PROPERTIES
    C_STANDARD 99 C_STANDARD_REQUIRED ON
    CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
  # MSVC deprecates the standard ("unsafe") and POSIX-named C library
  # functions used throughout; silence both classes of C4996.
  if(MSVC)
    target_compile_definitions(${target} PRIVATE
      _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
  endif()
  if(UNIX)
    target_link_libraries(${target} PRIVATE m)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# BLAKE3 (armor mode).  Fetched from the official repository, pinned to a
# release tag so upstream security fixes are taken by bumping XD3_BLAKE3_TAG.
# The C library lives in the repo's c/ subdirectory and exports the `blake3`
# target.
# ---------------------------------------------------------------------------
set(XD3_ARMOR_LIBRARIES "")
if(XD3_ARMOR)
  include(FetchContent)
  FetchContent_Declare(blake3
    GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
    GIT_TAG ${XD3_BLAKE3_TAG}
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR c)
  FetchContent_MakeAvailable(blake3)
  set(XD3_ARMOR_LIBRARIES blake3)
  message(STATUS "xdelta3: armor mode enabled (BLAKE3 ${XD3_BLAKE3_TAG})")
else()
  message(STATUS "xdelta3: armor mode disabled")
endif()

# ---------------------------------------------------------------------------
# xdelta3 - the command-line tool / library driver
# ---------------------------------------------------------------------------
add_executable(xdelta3 xdelta3.c)
xd3_configure_target(xdelta3)
target_compile_definitions(xdelta3 PRIVATE
  REGRESSION_TEST=1 SECONDARY_DJW=1 SECONDARY_FGK=1 XD3_MAIN=1 XD3_DEBUG=0
  ${XD3_PLATFORM_DEFS})
target_compile_options(xdelta3 PRIVATE ${XD3_WERROR_FLAGS})
target_link_libraries(xdelta3 PRIVATE ${XD3_LZMA_LIBRARIES})
if(XD3_ARMOR)
  target_compile_definitions(xdelta3 PRIVATE XD3_ARMOR=1)
  target_link_libraries(xdelta3 PRIVATE ${XD3_ARMOR_LIBRARIES})
endif()

# ---------------------------------------------------------------------------
# xdelta3decode - minimal decode-only build (no encoder, no secondary comp.)
# ---------------------------------------------------------------------------
add_executable(xdelta3decode xdelta3.c)
xd3_configure_target(xdelta3decode)
target_compile_definitions(xdelta3decode PRIVATE
  REGRESSION_TEST=0 SECONDARY_DJW=0 SECONDARY_FGK=0 SECONDARY_LZMA=0
  XD3_MAIN=1 XD3_ENCODER=0 XD3_STDIO=1 EXTERNAL_COMPRESSION=0 VCDIFF_TOOLS=0)
target_compile_options(xdelta3decode PRIVATE ${XD3_WERROR_FLAGS})

# ---------------------------------------------------------------------------
# Regression-test executables
# ---------------------------------------------------------------------------
if(XD3_BUILD_TESTS)
  # The C++ regression harness in testing/ is POSIX-only: it includes
  # <unistd.h> and uses the __typeof__ extension, neither of which MSVC
  # supports.  (The historical Visual Studio build never built it either.)
  # Skip it under MSVC; the built-in `xdelta3 test` still runs on Windows.
  if(NOT MSVC)
    add_executable(xdelta3regtest
      testing/regtest.cc
      testing/regtest_c.c)
    xd3_configure_target(xdelta3regtest)
    target_compile_definitions(xdelta3regtest PRIVATE
      REGRESSION_TEST=1 SECONDARY_DJW=1 SECONDARY_FGK=1
      XD3_MAIN=1 NOT_MAIN=1 XD3_DEBUG=1
      ${XD3_PLATFORM_DEFS})
    target_link_libraries(xdelta3regtest PRIVATE ${XD3_LZMA_LIBRARIES})

    add_executable(xdelta3checksum
      testing/checksum_test.cc
      testing/checksum_test_c.c)
    xd3_configure_target(xdelta3checksum)
    target_compile_definitions(xdelta3checksum PRIVATE
      REGRESSION_TEST=1 SECONDARY_DJW=1 SECONDARY_FGK=1
      XD3_MAIN=1 NOT_MAIN=1
      ${XD3_PLATFORM_DEFS})
    target_link_libraries(xdelta3checksum PRIVATE ${XD3_LZMA_LIBRARIES})
  endif()

  # xdelta3link references every public API symbol; building it verifies the
  # library links and catches signature drift in xdelta3.h.  It is a
  # compile/link check only -- it is not run, since it intentionally passes
  # NULL/uninitialized arguments.
  add_executable(xdelta3link linkxd3lib.c xdelta3.c)
  xd3_configure_target(xdelta3link)
  target_compile_definitions(xdelta3link PRIVATE
    REGRESSION_TEST=0 SECONDARY_DJW=1 SECONDARY_FGK=1 XD3_MAIN=0
    ${XD3_PLATFORM_DEFS})
  target_compile_options(xdelta3link PRIVATE ${XD3_WERROR_FLAGS})
  target_link_libraries(xdelta3link PRIVATE ${XD3_LZMA_LIBRARIES})

  enable_testing()
  add_test(NAME xdelta3_builtin_test COMMAND xdelta3 test)
  set_tests_properties(xdelta3_builtin_test PROPERTIES TIMEOUT 600)
endif()

install(TARGETS xdelta3 RUNTIME DESTINATION bin)
install(FILES xdelta3.h DESTINATION include)
install(FILES xdelta3.1 DESTINATION share/man/man1)
