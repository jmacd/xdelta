set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

# Download pre-built binaries from GitHub Releases
vcpkg_download_distfile(
    ARCHIVE
    URLS "https://github.com/loonghao/xdelta/releases/download/v${VERSION}/xdelta-${VERSION}-windows.zip"
    FILENAME "xdelta-${VERSION}-windows.zip"
    SHA512 "5255fb6d078ef182c7d0974ecf4db68aa5f2756f2f15f91d7dd0ee751821bdab5b8c8268c42eb21b4450b981637a7adb39d2342988737b2faa76341cf915afab"
)

# Extract the archive
vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    NO_REMOVE_ONE_LEVEL
)

# Install binaries
if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(ARCH_DIR "${SOURCE_PATH}/${VERSION}/x64-windows")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(ARCH_DIR "${SOURCE_PATH}/${VERSION}/x86-windows")
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

# Install include files
file(INSTALL "${ARCH_DIR}/include/xdelta3/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/xdelta3")

# Install debug libs
file(INSTALL "${ARCH_DIR}/lib/xdeltad.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")

# Install release libs
file(INSTALL "${ARCH_DIR}/lib/xdelta.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")

# Install debug tools
if(NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    file(INSTALL "${ARCH_DIR}/bin/xdelta3d.exe" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/tools/${PORT}")
endif()

# Install release tools
if(NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
    file(INSTALL "${ARCH_DIR}/bin/xdelta3.exe" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
endif()

# Handle copyright
file(INSTALL "${SOURCE_PATH}/${VERSION}/README.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

# Configure usage
configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)

