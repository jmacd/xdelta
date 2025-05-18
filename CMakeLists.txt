cmake_minimum_required(VERSION 3.14)

#-------------------------------------------------------------------------------------------
# vcpkg integration
#-------------------------------------------------------------------------------------------
# Automatically detect and use vcpkg toolchain if VCPKG_ROOT is defined
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
  message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Check if we're using vcpkg
if(DEFINED CMAKE_TOOLCHAIN_FILE)
  if(CMAKE_TOOLCHAIN_FILE MATCHES "vcpkg.cmake")
    set(USING_VCPKG TRUE)
    message(STATUS "Using vcpkg integration")

    # Support for vcpkg manifest mode (vcpkg.json)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
      message(STATUS "Detected vcpkg.json - using manifest mode")
    endif()
  endif()
endif()
# Set project information
project(xdelta
    VERSION 3.1.1
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
# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set CXX standard for tests
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configure static/dynamic runtime linking on Windows
if(MSVC AND XDELTA_USE_STATIC_RUNTIME)
    # Use static runtime
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
    endif()

    find_package(LibLZMA REQUIRED)
    message(STATUS "Found LibLZMA: ${LibLZMA_LIBRARIES}")
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
    elseif(USING_VCPKG)
        # vcpkg-specific handling
        message(STATUS "Using vcpkg-provided LibLZMA")

        # Determine the correct library name based on build type and linking mode
        set(LZMA_LIB_NAME "liblzma")
        if(MSVC)
            if(NOT BUILD_SHARED_LIBS OR LIBLZMA_USE_STATIC_LIBS)
                # Static library
                if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                    set(LZMA_LIB_NAME "liblzmad")
                endif()
            else()
                # Dynamic library
                if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                    set(LZMA_LIB_NAME "liblzma")
                endif()
            endif()
        endif()

        # Find the library in vcpkg installed directory
        find_library(VCPKG_LZMA_LIB
            NAMES ${LZMA_LIB_NAME} lzma
            PATHS "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib"
            NO_DEFAULT_PATH
        )

        if(VCPKG_LZMA_LIB)
            target_include_directories(xdelta PRIVATE
                "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
            target_link_libraries(xdelta PRIVATE ${VCPKG_LZMA_LIB})
            message(STATUS "Linking xdelta with vcpkg LibLZMA: ${VCPKG_LZMA_LIB}")
        else()
            message(WARNING "LibLZMA library not found in vcpkg. LZMA support may not work correctly.")
        endif()
    else()
        message(WARNING "LibLZMA not found. LZMA support may not work correctly.")
    endif()
endif()

if(WIN32)
    target_compile_definitions(xdelta PRIVATE
        _WIN32=1
        XD3_WIN32=1
        _CRT_SECURE_NO_WARNINGS
    )
endif()

#-------------------------------------------------------------------------------------------
# Executables
#-------------------------------------------------------------------------------------------
# Main xdelta3 executable
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
    elseif(USING_VCPKG AND VCPKG_LZMA_LIB)
        # Use the same vcpkg library we found for the main library
        target_include_directories(xdelta3 PRIVATE
            "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
        target_link_libraries(xdelta3 PRIVATE ${VCPKG_LZMA_LIB})
    endif()
endif()

if(WIN32)
    target_compile_definitions(xdelta3 PRIVATE
        _WIN32=1
        XD3_WIN32=1
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Optional test executables
if(XDELTA_BUILD_TESTS)
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
        elseif(USING_VCPKG AND VCPKG_LZMA_LIB)
            # Use the same vcpkg library we found for the main library
            target_include_directories(xdelta3regtest PRIVATE
                "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
            target_link_libraries(xdelta3regtest PRIVATE ${VCPKG_LZMA_LIB})
        endif()
    endif()

    if(WIN32)
        target_compile_definitions(xdelta3regtest PRIVATE
            _WIN32=1
            XD3_WIN32=1
            _CRT_SECURE_NO_WARNINGS
        )
    endif()

    # xdelta3decode
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
        elseif(USING_VCPKG AND VCPKG_LZMA_LIB)
            # Use the same vcpkg library we found for the main library
            target_include_directories(xdelta3decode PRIVATE
                "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include")
            target_link_libraries(xdelta3decode PRIVATE ${VCPKG_LZMA_LIB})
        endif()
    endif()

    if(WIN32)
        target_compile_definitions(xdelta3decode PRIVATE
            _WIN32=1
            XD3_WIN32=1
            _CRT_SECURE_NO_WARNINGS
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
