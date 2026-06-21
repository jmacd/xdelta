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

# Build and install the reusable xdelta3 core library (encoder + decoder +
# DJW secondary compression, XD3_MAIN=0).  This is the unity-build
# xdelta3.c compiled once as a library so other applications can embed the
# VCDIFF codec via the public xd3_* API in xdelta3.h.
option(XD3_BUILD_LIB "Build and install the xdelta3 core library" ON)

# FGK adaptive-Huffman secondary compression.  DJW (static Huffman) is the
# default and only secondary compressor xdelta3 selects on its own, so FGK is
# effectively unused; it is disabled by default in the tool and the library.
# The regression-test targets always build it so its code path stays covered.
option(XD3_SECONDARY_FGK "Build FGK adaptive-Huffman secondary compression" OFF)

# When ON, the library compiles liblzma secondary compression in and
# propagates the dependency to consumers (transitively linked for static
# builds).  When OFF (default), the library is dependency-free, matching the
# historical noinst library from PR #254.
option(XD3_LIB_LZMA "Compile liblzma secondary compression into the library" OFF)

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

# FetchContent dependencies are statically linked into xdelta3 but should not
# contribute their own install rules to `cmake --install`.  BLAKE3 in
# particular installs a libblake3.pc generated in the wrong directory under
# FetchContent, which aborts the install.  EXCLUDE_FROM_ALL on
# FetchContent_Declare drops a dependency's targets and install rules; it is
# only honored from CMake 3.28, so guard it and fall back to nothing on older
# versions.  It is applied only to BLAKE3: xz (liblzma) registers its own
# CTest tests, and excluding it from `all` would leave those test binaries
# unbuilt yet still registered, breaking the XD3_LZMA_FETCH test run.
set(XD3_FETCHCONTENT_EXCLUDE "")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
  set(XD3_FETCHCONTENT_EXCLUDE EXCLUDE_FROM_ALL)
endif()

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
    SOURCE_SUBDIR c
    ${XD3_FETCHCONTENT_EXCLUDE})
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
  REGRESSION_TEST=1 SECONDARY_DJW=1 SECONDARY_FGK=$<BOOL:${XD3_SECONDARY_FGK}>
  XD3_MAIN=1 XD3_DEBUG=0
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

  # Consumer smoke test: links the core library via its public API and runs a
  # real encode/decode round-trip, verifying the library is usable as built.
  if(XD3_BUILD_LIB)
    add_executable(xdelta3_lib_smoke testing/lib_smoke.c)
    target_link_libraries(xdelta3_lib_smoke PRIVATE xdelta3lib)
    set_target_properties(xdelta3_lib_smoke PROPERTIES
      C_STANDARD 99 C_STANDARD_REQUIRED ON)
    target_compile_options(xdelta3_lib_smoke PRIVATE ${XD3_WERROR_FLAGS})
    add_test(NAME xdelta3_lib_smoke COMMAND xdelta3_lib_smoke)
  endif()
endif()

install(TARGETS xdelta3 RUNTIME DESTINATION bin)
install(FILES xdelta3.h DESTINATION include)
install(FILES xdelta3.1 DESTINATION share/man/man1)

# ---------------------------------------------------------------------------
# xdelta3lib - the reusable core library (encoder + decoder, XD3_MAIN=0)
# ---------------------------------------------------------------------------
if(XD3_BUILD_LIB)
  # OUTPUT_NAME xdelta3 -> libxdelta3.a / libxdelta3.so; no clash with the
  # `xdelta3` executable.  Honors BUILD_SHARED_LIBS (default static).
  add_library(xdelta3lib xdelta3.c)
  add_library(xdelta3::xdelta3 ALIAS xdelta3lib)
  xd3_configure_target(xdelta3lib)
  set_target_properties(xdelta3lib PROPERTIES
    OUTPUT_NAME xdelta3
    EXPORT_NAME xdelta3
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

  # Public include dirs: the source tree at build time, <prefix>/include once
  # installed.  Consumers only need xdelta3.h.
  target_include_directories(xdelta3lib PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:include>")

  # The public header (xdelta3.h) sizes usize_t/xoff_t from these macros, which
  # normally come from config.h (HAVE_CONFIG_H).  Export them on the library's
  # INTERFACE so consumers get an ABI-compatible header without needing
  # config.h.  These reflect this build's configuration; see README for the
  # ABI implications of XD3_LARGESIZET.
  set(XD3_LIB_ABI_DEFS
    SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
    SIZEOF_UNSIGNED_INT=${SIZEOF_UNSIGNED_INT}
    SIZEOF_UNSIGNED_LONG=${SIZEOF_UNSIGNED_LONG}
    SIZEOF_UNSIGNED_LONG_LONG=${SIZEOF_UNSIGNED_LONG_LONG}
    XD3_USE_LARGESIZET=$<BOOL:${XD3_LARGESIZET}>)
  target_compile_definitions(xdelta3lib INTERFACE ${XD3_LIB_ABI_DEFS})

  set(XD3_LIB_DEFS REGRESSION_TEST=0 SECONDARY_DJW=1
    SECONDARY_FGK=$<BOOL:${XD3_SECONDARY_FGK}>
    XD3_MAIN=0 XD3_DEBUG=0)
  if(XD3_LIB_LZMA AND HAVE_LZMA_H)
    list(APPEND XD3_LIB_DEFS SECONDARY_LZMA=1)
    target_link_libraries(xdelta3lib PUBLIC ${XD3_LZMA_LIBRARIES})
  else()
    list(APPEND XD3_LIB_DEFS SECONDARY_LZMA=0)
  endif()
  target_compile_definitions(xdelta3lib PRIVATE ${XD3_LIB_DEFS} ${XD3_PLATFORM_DEFS})
  target_compile_options(xdelta3lib PRIVATE ${XD3_WERROR_FLAGS})

  # Windows DLLs need exported symbols; the public API has no __declspec
  # annotations, so export everything when building shared on Windows.
  if(WIN32 AND BUILD_SHARED_LIBS)
    set_target_properties(xdelta3lib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()

  # Install + CMake package config so downstream can find_package(xdelta3).
  include(CMakePackageConfigHelpers)
  include(GNUInstallDirs)
  install(TARGETS xdelta3lib EXPORT xdelta3Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(EXPORT xdelta3Targets
    FILE xdelta3Targets.cmake
    NAMESPACE xdelta3::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xdelta3)
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/xdelta3Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/xdelta3Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xdelta3)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/xdelta3ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/xdelta3Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/xdelta3ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xdelta3)

  # pkg-config for non-CMake consumers.
  if(XD3_LIB_LZMA AND HAVE_LZMA_H)
    set(XD3_PC_REQUIRES "liblzma")
  else()
    set(XD3_PC_REQUIRES "")
  endif()
  if(XD3_LARGESIZET)
    set(XD3_PC_LARGESIZET 1)
  else()
    set(XD3_PC_LARGESIZET 0)
  endif()
  set(XD3_PC_CFLAGS
    "-DSIZEOF_SIZE_T=${SIZEOF_SIZE_T} -DSIZEOF_UNSIGNED_INT=${SIZEOF_UNSIGNED_INT} -DSIZEOF_UNSIGNED_LONG=${SIZEOF_UNSIGNED_LONG} -DSIZEOF_UNSIGNED_LONG_LONG=${SIZEOF_UNSIGNED_LONG_LONG} -DXD3_USE_LARGESIZET=${XD3_PC_LARGESIZET}")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/xdelta3.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/xdelta3.pc" @ONLY)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xdelta3.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
