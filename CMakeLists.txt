cmake_minimum_required(VERSION 3.14)

#-------------------------------------------------------------------------------------------
# vcpkg integration
#-------------------------------------------------------------------------------------------
# Automatically detect and use vcpkg toolchain if VCPKG_ROOT is defined
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
  message(STATUS "Using vcpkg toolchain file from VCPKG_ROOT: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Check if we're using vcpkg
if(DEFINED CMAKE_TOOLCHAIN_FILE)
  if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
    set(USING_VCPKG TRUE)
    message(STATUS "Using vcpkg integration with toolchain: ${CMAKE_TOOLCHAIN_FILE}")

    # Support for vcpkg manifest mode (vcpkg.json)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
      message(STATUS "Detected vcpkg.json - using manifest mode")
    endif()
  endif()
else()
  # If toolchain file is not defined, try to find it in common locations
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file" FORCE)
    set(USING_VCPKG TRUE)
    message(STATUS "Found vcpkg toolchain in project directory: ${CMAKE_TOOLCHAIN_FILE}")
  endif()
endif()
# Set project information
project(xdelta
    VERSION 3.1.0
    DESCRIPTION "Xdelta: a binary diff, differential compression tools"
    HOMEPAGE_URL "http://xdelta.org/"
    LANGUAGES C CXX
)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(XDELTA_BUILD_TESTS "Build xdelta tests" OFF)
option(XDELTA_ENABLE_LZMA "Enable LZMA compression support" OFF)
option(XDELTA_ENABLE_DOCS "Build documentation" OFF)
option(XDELTA_USE_STATIC_RUNTIME "Link with static C runtime (Windows only)" OFF)

# Include CMake modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

#-------------------------------------------------------------------------------------------
# Compiler settings
#-------------------------------------------------------------------------------------------
# Set C standard (use C11 when compiling as C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set C++ standard to C++14 for better cross-platform compatibility
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force C++ compilation for better cross-platform compatibility
# Note: We need to set this after the sources are defined, so we'll do it later
# set_source_files_properties(${XDELTA_SOURCES} PROPERTIES LANGUAGE CXX)

# Add compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    # Use C++14 for all files
    add_compile_options(-std=c++14)

    # Add additional warning suppressions for C++ compilation of C code
    add_compile_options(-Wno-deprecated -Wno-write-strings)
elseif(MSVC)
    # MSVC 2015 and later support C++14
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
        message(WARNING "MSVC version may not fully support C++14 features. Consider upgrading to Visual Studio 2015 or later.")
    endif()

    # Disable specific MSVC warnings for C++ compilation of C code
    add_compile_options(
        /wd4996  # Disable deprecated function warnings
        /wd4244  # Disable conversion warnings
        /wd4267  # Disable size_t to smaller type conversion warnings
        /wd4334  # Disable 32-bit shift warnings
        /wd4311  # Disable pointer truncation warnings
        /wd4090  # Disable const qualifier warnings
        /wd4101  # Disable unreferenced local variable warnings
        /wd4018  # Disable signed/unsigned mismatch warnings
        /wd4068  # Disable unknown pragma warnings
    )

    # Enable multi-processor compilation
    add_compile_options(/MP)

    # Treat warnings as errors for cleaner code
    # add_compile_options(/WX) # Uncomment if you want to enforce warnings as errors
endif()

# Configure platform-specific settings
if(MSVC AND XDELTA_USE_STATIC_RUNTIME)
    # Use static runtime on Windows
    foreach(flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()

    # If using vcpkg, ensure static libraries are used
    if(USING_VCPKG)
        set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
        message(STATUS "Using static vcpkg libraries with triplet: ${VCPKG_TARGET_TRIPLET}")
    endif()
endif()

# macOS specific settings
if(APPLE)
    # Set minimum macOS version to ensure compatibility
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")

    # Add macOS-specific compiler flags
    add_compile_options(-Wall -Wextra)

    # If using vcpkg on macOS, set the appropriate triplet
    if(USING_VCPKG AND NOT DEFINED VCPKG_TARGET_TRIPLET)
        if(BUILD_SHARED_LIBS)
            set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "")
        else()
            set(VCPKG_TARGET_TRIPLET "x64-osx-static" CACHE STRING "")
        endif()
        message(STATUS "Using vcpkg libraries with triplet: ${VCPKG_TARGET_TRIPLET}")
    endif()
endif()

#-------------------------------------------------------------------------------------------
# Dependencies
#-------------------------------------------------------------------------------------------
# Find dependencies
if(XDELTA_ENABLE_LZMA)
    # When using vcpkg, we can specify static/dynamic linking preference
    if(USING_VCPKG)
        if(BUILD_SHARED_LIBS)
            set(LIBLZMA_USE_STATIC_LIBS OFF)
        else()
            set(LIBLZMA_USE_STATIC_LIBS ON)
        endif()

        message(STATUS "Using vcpkg for liblzma dependency")

        # Find LibLZMA through vcpkg
        find_package(LibLZMA REQUIRED)
        message(STATUS "Found LibLZMA via vcpkg: ${LibLZMA_LIBRARIES}")
        message(STATUS "LibLZMA include dirs: ${LibLZMA_INCLUDE_DIRS}")
    else()
        # Find pkg-config for all platforms when not using vcpkg
        find_package(PkgConfig QUIET)

        # On macOS, try to find liblzma from Homebrew or MacPorts first
        if(APPLE)
            # Check for Homebrew xz package (which provides liblzma)
            execute_process(
                COMMAND brew --prefix xz
                RESULT_VARIABLE BREW_XZ_RESULT
                OUTPUT_VARIABLE BREW_XZ_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(BREW_XZ_RESULT EQUAL 0 AND EXISTS "${BREW_XZ_PREFIX}")
                message(STATUS "Found Homebrew xz (liblzma) at ${BREW_XZ_PREFIX}")
                list(APPEND CMAKE_PREFIX_PATH "${BREW_XZ_PREFIX}")

                # Explicitly set include and library paths for Homebrew
                set(LIBLZMA_INCLUDE_DIR "${BREW_XZ_PREFIX}/include" CACHE PATH "liblzma include directory")

                # Check for the library in various locations
                if(EXISTS "${BREW_XZ_PREFIX}/lib/liblzma.dylib")
                    set(LIBLZMA_LIBRARY "${BREW_XZ_PREFIX}/lib/liblzma.dylib" CACHE FILEPATH "liblzma library")
                elseif(EXISTS "${BREW_XZ_PREFIX}/lib/liblzma.a")
                    set(LIBLZMA_LIBRARY "${BREW_XZ_PREFIX}/lib/liblzma.a" CACHE FILEPATH "liblzma library")
                # Also check for lib64 directory which might exist on some systems
                elseif(EXISTS "${BREW_XZ_PREFIX}/lib64/liblzma.dylib")
                    set(LIBLZMA_LIBRARY "${BREW_XZ_PREFIX}/lib64/liblzma.dylib" CACHE FILEPATH "liblzma library")
                elseif(EXISTS "${BREW_XZ_PREFIX}/lib64/liblzma.a")
                    set(LIBLZMA_LIBRARY "${BREW_XZ_PREFIX}/lib64/liblzma.a" CACHE FILEPATH "liblzma library")
                endif()

                # If we found both include and library, mark as found
                if(LIBLZMA_INCLUDE_DIR AND LIBLZMA_LIBRARY)
                    set(LibLZMA_FOUND TRUE)
                    set(LibLZMA_INCLUDE_DIRS ${LIBLZMA_INCLUDE_DIR})
                    set(LibLZMA_LIBRARIES ${LIBLZMA_LIBRARY})
                    message(STATUS "Manually set LibLZMA: ${LibLZMA_LIBRARIES}")
                endif()
            endif()

            # Check for MacPorts if Homebrew not found
            if(NOT LibLZMA_FOUND AND EXISTS "/opt/local/include/lzma.h")
                message(STATUS "Found MacPorts liblzma")
                list(APPEND CMAKE_PREFIX_PATH "/opt/local")

                # Explicitly set include and library paths for MacPorts
                set(LIBLZMA_INCLUDE_DIR "/opt/local/include" CACHE PATH "liblzma include directory")

                if(EXISTS "/opt/local/lib/liblzma.dylib")
                    set(LIBLZMA_LIBRARY "/opt/local/lib/liblzma.dylib" CACHE FILEPATH "liblzma library")
                elseif(EXISTS "/opt/local/lib/liblzma.a")
                    set(LIBLZMA_LIBRARY "/opt/local/lib/liblzma.a" CACHE FILEPATH "liblzma library")
                endif()

                if(LIBLZMA_INCLUDE_DIR AND LIBLZMA_LIBRARY)
                    set(LibLZMA_FOUND TRUE)
                    set(LibLZMA_INCLUDE_DIRS ${LIBLZMA_INCLUDE_DIR})
                    set(LibLZMA_LIBRARIES ${LIBLZMA_LIBRARY})
                    message(STATUS "Manually set LibLZMA: ${LibLZMA_LIBRARIES}")
                endif()
            endif()

            # Use pkg-config as fallback
            if(NOT LibLZMA_FOUND AND PKG_CONFIG_FOUND)
                pkg_check_modules(PC_LIBLZMA QUIET liblzma)
                if(PC_LIBLZMA_FOUND)
                    message(STATUS "Found liblzma via pkg-config")
                    set(LibLZMA_INCLUDE_DIRS ${PC_LIBLZMA_INCLUDE_DIRS})
                    set(LibLZMA_LIBRARIES ${PC_LIBLZMA_LIBRARIES})
                    set(LibLZMA_FOUND TRUE)
                endif()
            endif()
        endif()

        # Only try to find LibLZMA if we haven't already found it manually
        if(NOT LibLZMA_FOUND)
            find_package(LibLZMA REQUIRED)
            message(STATUS "Found LibLZMA via find_package: ${LibLZMA_LIBRARIES}")
        else()
            message(STATUS "Using manually configured LibLZMA: ${LibLZMA_LIBRARIES}")
        endif()
    endif()
endif()

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
set(XDELTA_SOURCES "xdelta3/xdelta3.c")
set(XDELTA_HEADERS
    "xdelta3/xdelta3.h"
    "xdelta3/xdelta3-blkcache.h"
    "xdelta3/xdelta3-cfgs.h"
    "xdelta3/xdelta3-decode.h"
    "xdelta3/xdelta3-djw.h"
    "xdelta3/xdelta3-fgk.h"
    "xdelta3/xdelta3-hash.h"
    "xdelta3/xdelta3-internal.h"
    "xdelta3/xdelta3-list.h"
    "xdelta3/xdelta3-lzma.h"
    "xdelta3/xdelta3-main.h"
    "xdelta3/xdelta3-merge.h"
    "xdelta3/xdelta3-second.h"
    "xdelta3/xdelta3-test.h"
)

# Force C++ compilation for better cross-platform compatibility
set_source_files_properties(${XDELTA_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(xdelta ${XDELTA_SOURCES} ${XDELTA_HEADERS})
add_library(xdelta::xdelta ALIAS xdelta)

# Set target properties
set_target_properties(xdelta PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${XDELTA_HEADERS}"
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)

#-------------------------------------------------------------------------------------------
# Include dirs and compile definitions
#-------------------------------------------------------------------------------------------
target_include_directories(xdelta
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/xdelta3>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/xdelta3>
)

# Determine size_t and other platform-specific settings
include(CheckTypeSize)
check_type_size("size_t" SIZEOF_SIZE_T)
check_type_size("unsigned int" SIZEOF_UNSIGNED_INT)
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)

# Ensure the values are defined as numeric values, not strings
# This fixes static_assert issues on Linux and macOS
if(SIZEOF_SIZE_T)
    math(EXPR SIZEOF_SIZE_T "${SIZEOF_SIZE_T}")
else()
    # Fallback for platforms where check_type_size fails
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SIZEOF_SIZE_T 8)
    else()
        set(SIZEOF_SIZE_T 4)
    endif()
endif()

if(SIZEOF_UNSIGNED_INT)
    math(EXPR SIZEOF_UNSIGNED_INT "${SIZEOF_UNSIGNED_INT}")
else()
    # Most platforms have 4-byte unsigned int
    set(SIZEOF_UNSIGNED_INT 4)
endif()

if(SIZEOF_UNSIGNED_LONG)
    math(EXPR SIZEOF_UNSIGNED_LONG "${SIZEOF_UNSIGNED_LONG}")
else()
    # On most 64-bit Unix/Linux/macOS, long is 8 bytes
    # On Windows and 32-bit systems, it's typically 4 bytes
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32)
        set(SIZEOF_UNSIGNED_LONG 8)
    else()
        set(SIZEOF_UNSIGNED_LONG 4)
    endif()
endif()

if(SIZEOF_UNSIGNED_LONG_LONG)
    math(EXPR SIZEOF_UNSIGNED_LONG_LONG "${SIZEOF_UNSIGNED_LONG_LONG}")
else()
    # unsigned long long is almost always 8 bytes
    set(SIZEOF_UNSIGNED_LONG_LONG 8)
endif()

# Print the detected sizes for debugging
message(STATUS "SIZEOF_SIZE_T: ${SIZEOF_SIZE_T}")
message(STATUS "SIZEOF_UNSIGNED_INT: ${SIZEOF_UNSIGNED_INT}")
message(STATUS "SIZEOF_UNSIGNED_LONG: ${SIZEOF_UNSIGNED_LONG}")
message(STATUS "SIZEOF_UNSIGNED_LONG_LONG: ${SIZEOF_UNSIGNED_LONG_LONG}")

# Set compile definitions
target_compile_definitions(xdelta
    PRIVATE
        SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
        SIZEOF_UNSIGNED_INT=${SIZEOF_UNSIGNED_INT}
        SIZEOF_UNSIGNED_LONG=${SIZEOF_UNSIGNED_LONG}
        SIZEOF_UNSIGNED_LONG_LONG=${SIZEOF_UNSIGNED_LONG_LONG}
        XD3_USE_LARGEFILE64=1
        SECONDARY_DJW=1
        SECONDARY_FGK=1
)

if(XDELTA_ENABLE_LZMA)
    # Add LZMA support
    target_compile_definitions(xdelta PRIVATE SECONDARY_LZMA=1)

    # Add static API definition if using static LZMA
    if(NOT BUILD_SHARED_LIBS OR LIBLZMA_USE_STATIC_LIBS)
        target_compile_definitions(xdelta PRIVATE LZMA_API_STATIC)
    endif()

    # Link against LZMA libraries
    if(LibLZMA_FOUND)
        # Standard CMake find_package result
        target_include_directories(xdelta PRIVATE ${LibLZMA_INCLUDE_DIRS})
        target_link_libraries(xdelta PRIVATE ${LibLZMA_LIBRARIES})
        message(STATUS "Linking xdelta with LibLZMA: ${LibLZMA_LIBRARIES}")
    else()
        message(WARNING "LibLZMA not found. LZMA support may not work correctly.")
    endif()
endif()

if(WIN32)
    target_compile_definitions(xdelta PRIVATE
        _WIN32=1
        XD3_WIN32=1
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX  # Prevent Windows.h from defining min/max macros
        WIN32_LEAN_AND_MEAN  # Exclude rarely-used Windows headers
    )
elseif(APPLE)
    target_compile_definitions(xdelta PRIVATE
        _DARWIN_C_SOURCE
        _XOPEN_SOURCE=600
        _DARWIN_BETTER_REALPATH  # Use better realpath implementation
    )
    target_compile_options(xdelta PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-function
        -Wno-error=implicit-function-declaration
        -Wno-c++11-narrowing     # Suppress narrowing conversion warnings
        -Wno-format-security     # Suppress format security warnings
        -Wno-unused-variable     # Suppress unused variable warnings
        -Wno-unused-value        # Suppress unused value warnings
        -Wno-incompatible-pointer-types  # Suppress pointer type warnings
    )
elseif(UNIX)
    # Linux and other Unix-like systems
    target_compile_definitions(xdelta PRIVATE
        _GNU_SOURCE
        _XOPEN_SOURCE=600
        _LARGEFILE64_SOURCE  # Support for large files
    )
    target_compile_options(xdelta PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-function
        -Wno-error=implicit-function-declaration
        -Wno-write-strings   # Suppress string literal conversion warnings
        -Wno-format-security # Suppress format security warnings
        -Wno-unused-result   # Suppress unused result warnings
        -Wno-unused-variable # Suppress unused variable warnings
        -Wno-unused-but-set-variable # Suppress unused but set variable warnings
        -Wno-narrowing       # Suppress narrowing conversion warnings
    )
endif()

#-------------------------------------------------------------------------------------------
# Executables
#-------------------------------------------------------------------------------------------
# Main xdelta3 executable
# Use the same source file but set it to C++ for this target as well
set_source_files_properties(${XDELTA_SOURCES} PROPERTIES LANGUAGE CXX)
add_executable(xdelta3 ${XDELTA_SOURCES})
target_include_directories(xdelta3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/xdelta3)
target_compile_definitions(xdelta3
    PRIVATE
        XD3_MAIN=1
        SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
        SIZEOF_UNSIGNED_INT=${SIZEOF_UNSIGNED_INT}
        SIZEOF_UNSIGNED_LONG=${SIZEOF_UNSIGNED_LONG}
        SIZEOF_UNSIGNED_LONG_LONG=${SIZEOF_UNSIGNED_LONG_LONG}
        XD3_USE_LARGEFILE64=1
        SECONDARY_DJW=1
        SECONDARY_FGK=1
)

if(XDELTA_ENABLE_LZMA)
    # Add LZMA support
    target_compile_definitions(xdelta3 PRIVATE SECONDARY_LZMA=1)

    # Add static API definition if using static LZMA
    if(NOT BUILD_SHARED_LIBS OR LIBLZMA_USE_STATIC_LIBS)
        target_compile_definitions(xdelta3 PRIVATE LZMA_API_STATIC)
    endif()

    # Link against LZMA libraries
    if(LibLZMA_FOUND)
        # Standard CMake find_package result
        target_include_directories(xdelta3 PRIVATE ${LibLZMA_INCLUDE_DIRS})
        target_link_libraries(xdelta3 PRIVATE ${LibLZMA_LIBRARIES})
    endif()
endif()

if(WIN32)
    target_compile_definitions(xdelta3 PRIVATE
        _WIN32=1
        XD3_WIN32=1
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX  # Prevent Windows.h from defining min/max macros
        WIN32_LEAN_AND_MEAN  # Exclude rarely-used Windows headers
    )
elseif(APPLE)
    target_compile_definitions(xdelta3 PRIVATE
        _DARWIN_C_SOURCE
        _XOPEN_SOURCE=600
        _DARWIN_BETTER_REALPATH  # Use better realpath implementation
    )
    target_compile_options(xdelta3 PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-function
        -Wno-error=implicit-function-declaration
        -Wno-c++11-narrowing     # Suppress narrowing conversion warnings
        -Wno-format-security     # Suppress format security warnings
        -Wno-unused-variable     # Suppress unused variable warnings
        -Wno-unused-value        # Suppress unused value warnings
        -Wno-incompatible-pointer-types  # Suppress pointer type warnings
    )
elseif(UNIX)
    # Linux and other Unix-like systems
    target_compile_definitions(xdelta3 PRIVATE
        _GNU_SOURCE
        _XOPEN_SOURCE=600
        _LARGEFILE64_SOURCE  # Support for large files
    )
    target_compile_options(xdelta3 PRIVATE
        -Wno-deprecated-declarations
        -Wno-unused-function
        -Wno-error=implicit-function-declaration
        -Wno-write-strings   # Suppress string literal conversion warnings
        -Wno-format-security # Suppress format security warnings
        -Wno-unused-result   # Suppress unused result warnings
        -Wno-unused-variable # Suppress unused variable warnings
        -Wno-unused-but-set-variable # Suppress unused but set variable warnings
        -Wno-narrowing       # Suppress narrowing conversion warnings
    )
endif()

# Optional test executables
if(XDELTA_BUILD_TESTS)
    # Set C++ language for test source files
    set_source_files_properties(${XDELTA_SOURCES} PROPERTIES LANGUAGE CXX)
    set_source_files_properties(xdelta3/testing/regtest_c.c PROPERTIES LANGUAGE CXX)

    # xdelta3regtest
    add_executable(xdelta3regtest
        ${XDELTA_SOURCES}
        xdelta3/testing/regtest.cc
        xdelta3/testing/regtest_c.c
    )
    target_include_directories(xdelta3regtest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/xdelta3)
    target_compile_definitions(xdelta3regtest
        PRIVATE
            REGRESSION_TEST=1
            XD3_MAIN=1
            SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
            SIZEOF_UNSIGNED_INT=${SIZEOF_UNSIGNED_INT}
            SIZEOF_UNSIGNED_LONG=${SIZEOF_UNSIGNED_LONG}
            SIZEOF_UNSIGNED_LONG_LONG=${SIZEOF_UNSIGNED_LONG_LONG}
            XD3_USE_LARGEFILE64=1
            SECONDARY_DJW=1
            SECONDARY_FGK=1
    )

    if(XDELTA_ENABLE_LZMA)
        # Add LZMA support
        target_compile_definitions(xdelta3regtest PRIVATE SECONDARY_LZMA=1)

        # Add static API definition if using static LZMA
        if(NOT BUILD_SHARED_LIBS OR LIBLZMA_USE_STATIC_LIBS)
            target_compile_definitions(xdelta3regtest PRIVATE LZMA_API_STATIC)
        endif()

        # Link against LZMA libraries
        if(LibLZMA_FOUND)
            # Standard CMake find_package result
            target_include_directories(xdelta3regtest PRIVATE ${LibLZMA_INCLUDE_DIRS})
            target_link_libraries(xdelta3regtest PRIVATE ${LibLZMA_LIBRARIES})
        endif()
    endif()

    if(WIN32)
        target_compile_definitions(xdelta3regtest PRIVATE
            _WIN32=1
            XD3_WIN32=1
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX  # Prevent Windows.h from defining min/max macros
            WIN32_LEAN_AND_MEAN  # Exclude rarely-used Windows headers
        )
    elseif(APPLE)
        target_compile_definitions(xdelta3regtest PRIVATE
            _DARWIN_C_SOURCE
            _XOPEN_SOURCE=600
            _DARWIN_BETTER_REALPATH  # Use better realpath implementation
        )
        target_compile_options(xdelta3regtest PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-function
            -Wno-error=implicit-function-declaration
            -Wno-c++11-narrowing     # Suppress narrowing conversion warnings
            -Wno-format-security     # Suppress format security warnings
            -Wno-unused-variable     # Suppress unused variable warnings
            -Wno-unused-value        # Suppress unused value warnings
            -Wno-incompatible-pointer-types  # Suppress pointer type warnings
        )
    elseif(UNIX)
        # Linux and other Unix-like systems
        target_compile_definitions(xdelta3regtest PRIVATE
            _GNU_SOURCE
            _XOPEN_SOURCE=600
            _LARGEFILE64_SOURCE  # Support for large files
        )
        target_compile_options(xdelta3regtest PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-function
            -Wno-error=implicit-function-declaration
            -Wno-write-strings   # Suppress string literal conversion warnings
            -Wno-format-security # Suppress format security warnings
            -Wno-unused-result   # Suppress unused result warnings
            -Wno-unused-variable # Suppress unused variable warnings
            -Wno-unused-but-set-variable # Suppress unused but set variable warnings
            -Wno-narrowing       # Suppress narrowing conversion warnings
        )
    endif()

    # xdelta3decode
    set_source_files_properties(${XDELTA_SOURCES} PROPERTIES LANGUAGE CXX)
    add_executable(xdelta3decode ${XDELTA_SOURCES})
    target_include_directories(xdelta3decode PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/xdelta3)
    target_compile_definitions(xdelta3decode
        PRIVATE
            NOT_MAIN=1
            XD3_MAIN=1
            SIZEOF_SIZE_T=${SIZEOF_SIZE_T}
            SIZEOF_UNSIGNED_INT=${SIZEOF_UNSIGNED_INT}
            SIZEOF_UNSIGNED_LONG=${SIZEOF_UNSIGNED_LONG}
            SIZEOF_UNSIGNED_LONG_LONG=${SIZEOF_UNSIGNED_LONG_LONG}
            XD3_USE_LARGEFILE64=1
            SECONDARY_DJW=1
            SECONDARY_FGK=1
    )

    if(XDELTA_ENABLE_LZMA)
        # Add LZMA support
        target_compile_definitions(xdelta3decode PRIVATE SECONDARY_LZMA=1)

        # Add static API definition if using static LZMA
        if(NOT BUILD_SHARED_LIBS OR LIBLZMA_USE_STATIC_LIBS)
            target_compile_definitions(xdelta3decode PRIVATE LZMA_API_STATIC)
        endif()

        # Link against LZMA libraries
        if(LibLZMA_FOUND)
            # Standard CMake find_package result
            target_include_directories(xdelta3decode PRIVATE ${LibLZMA_INCLUDE_DIRS})
            target_link_libraries(xdelta3decode PRIVATE ${LibLZMA_LIBRARIES})
        endif()
    endif()

    if(WIN32)
        target_compile_definitions(xdelta3decode PRIVATE
            _WIN32=1
            XD3_WIN32=1
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX  # Prevent Windows.h from defining min/max macros
            WIN32_LEAN_AND_MEAN  # Exclude rarely-used Windows headers
        )
    elseif(APPLE)
        target_compile_definitions(xdelta3decode PRIVATE
            _DARWIN_C_SOURCE
            _XOPEN_SOURCE=600
            _DARWIN_BETTER_REALPATH  # Use better realpath implementation
        )
        target_compile_options(xdelta3decode PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-function
            -Wno-error=implicit-function-declaration
            -Wno-c++11-narrowing     # Suppress narrowing conversion warnings
            -Wno-format-security     # Suppress format security warnings
            -Wno-unused-variable     # Suppress unused variable warnings
            -Wno-unused-value        # Suppress unused value warnings
            -Wno-incompatible-pointer-types  # Suppress pointer type warnings
        )
    elseif(UNIX)
        # Linux and other Unix-like systems
        target_compile_definitions(xdelta3decode PRIVATE
            _GNU_SOURCE
            _XOPEN_SOURCE=600
            _LARGEFILE64_SOURCE  # Support for large files
        )
        target_compile_options(xdelta3decode PRIVATE
            -Wno-deprecated-declarations
            -Wno-unused-function
            -Wno-error=implicit-function-declaration
            -Wno-write-strings   # Suppress string literal conversion warnings
            -Wno-format-security # Suppress format security warnings
            -Wno-unused-result   # Suppress unused result warnings
            -Wno-unused-variable # Suppress unused variable warnings
            -Wno-unused-but-set-variable # Suppress unused but set variable warnings
            -Wno-narrowing       # Suppress narrowing conversion warnings
        )
    endif()
endif()

#-------------------------------------------------------------------------------------------
# Installation
#-------------------------------------------------------------------------------------------
# Install library
install(
    TARGETS xdelta
    EXPORT xdeltaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xdelta3
)

# Install executables
install(
    TARGETS xdelta3
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(XDELTA_BUILD_TESTS)
    install(
        TARGETS xdelta3regtest xdelta3decode
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# Install man page
install(
    FILES xdelta3/xdelta3.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

#-------------------------------------------------------------------------------------------
# CMake package configuration
#-------------------------------------------------------------------------------------------
# Generate and install package configuration files
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/xdeltaConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Export targets
export(
    EXPORT xdeltaTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/xdeltaTargets.cmake"
    NAMESPACE xdelta::
)

# Configure the config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/xdeltaConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/xdeltaConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xdelta
)

# Install the config, configversion and custom find modules
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/xdeltaConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/xdeltaConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xdelta
)

# Install the export
install(
    EXPORT xdeltaTargets
    FILE xdeltaTargets.cmake
    NAMESPACE xdelta::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/xdelta
)

#-------------------------------------------------------------------------------------------
# Testing
#-------------------------------------------------------------------------------------------
if(XDELTA_BUILD_TESTS)
    enable_testing()

    # Basic functionality test
    add_test(
        NAME xdelta3_selftest
        COMMAND xdelta3 test
    )

    # Regression tests
    if(NOT WIN32)
        add_test(
            NAME xdelta3_regtest
            COMMAND xdelta3regtest
        )
    endif()
endif()
