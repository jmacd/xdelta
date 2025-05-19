#!/bin/bash
# Script to verify artifacts
# Usage: ./verify-artifacts.sh <artifacts_dir> <platform> [<arch>]

# Check if required arguments are provided
if [ $# -lt 2 ]; then
    echo "Usage: $0 <artifacts_dir> <platform> [<arch>]"
    echo "  platform: windows, linux, macos, windows-archive, linux-archive, macos-archive"
    echo "  arch: x64, x86 (only relevant for Windows, defaults to x64)"
    exit 1
fi

ARTIFACTS_DIR="$1"
PLATFORM="$2"
ARCH="${3:-x64}"  # Default to x64 if not provided

echo "Verifying artifacts in: $ARTIFACTS_DIR"
echo "Platform: $PLATFORM"
echo "Architecture: $ARCH"

# Check if artifacts directory exists
if [ ! -d "$ARTIFACTS_DIR" ] && [[ "$PLATFORM" != *-archive ]]; then
    echo "❌ Artifacts directory not found: $ARTIFACTS_DIR"
    exit 1
fi

# Check if archive file exists
if [[ "$PLATFORM" == *-archive ]] && [ ! -f "$ARTIFACTS_DIR" ]; then
    echo "❌ Archive file not found: $ARTIFACTS_DIR"
    exit 1
fi

# List all files in the artifacts directory
if [[ "$PLATFORM" != *-archive ]]; then
    echo "Files in artifacts directory:"
    ls -la "$ARTIFACTS_DIR"
fi

# Function to verify Windows artifacts
verify_windows_artifacts() {
    local dir="$1"

    # Check for xdelta3.exe
    if [ ! -f "$dir/xdelta3.exe" ]; then
        echo "❌ xdelta3.exe not found in $dir"
        return 1
    fi

    # Check for README.txt
    if [ ! -f "$dir/README.txt" ]; then
        echo "❌ README.txt not found in $dir"
        return 1
    fi

    # Check for liblzma.dll
    if [ ! -f "$dir/liblzma.dll" ]; then
        echo "⚠️ liblzma.dll not found in $dir (may be statically linked)"
    fi

    echo "✅ All required Windows $ARCH files found"
    return 0
}

# Function to verify Linux artifacts
verify_linux_artifacts() {
    local dir="$1"

    # Check for xdelta3
    if [ ! -f "$dir/xdelta3" ]; then
        echo "❌ xdelta3 not found in $dir"
        return 1
    fi

    # Check for README.txt
    if [ ! -f "$dir/README.txt" ]; then
        echo "❌ README.txt not found in $dir"
        return 1
    fi

    echo "✅ All required Linux files found"
    return 0
}

# Function to verify macOS artifacts
verify_macos_artifacts() {
    local dir="$1"

    # Check for README.txt (macOS build is disabled, so only README is required)
    if [ ! -f "$dir/README.txt" ]; then
        echo "❌ README.txt not found in $dir"
        return 1
    fi

    echo "✅ All required macOS files found (note: macOS build is disabled)"
    return 0
}

# Function to verify archive artifacts
verify_archive() {
    local archive_path="$1"
    local platform="${2%-archive}"  # Remove -archive suffix
    local temp_dir="$(mktemp -d)"

    echo "Extracting archive to temporary directory: $temp_dir"

    # Extract based on archive type
    if [[ "$archive_path" == *.zip ]]; then
        unzip -q "$archive_path" -d "$temp_dir"
    elif [[ "$archive_path" == *.tar.gz ]]; then
        tar -xzf "$archive_path" -C "$temp_dir"
    else
        echo "❌ Unknown archive format: $archive_path"
        return 1
    fi

    # Verify the extracted contents
    case "$platform" in
        windows)
            verify_windows_artifacts "$temp_dir"
            result=$?
            ;;
        linux)
            verify_linux_artifacts "$temp_dir"
            result=$?
            ;;
        macos)
            verify_macos_artifacts "$temp_dir"
            result=$?
            ;;
        *)
            echo "❌ Unknown platform: $platform"
            result=1
            ;;
    esac

    # Clean up
    rm -rf "$temp_dir"
    return $result
}

# Verify artifacts based on platform
case "$PLATFORM" in
    windows)
        verify_windows_artifacts "$ARTIFACTS_DIR"
        exit $?
        ;;
    linux)
        verify_linux_artifacts "$ARTIFACTS_DIR"
        exit $?
        ;;
    macos)
        verify_macos_artifacts "$ARTIFACTS_DIR"
        exit $?
        ;;
    windows-archive|linux-archive|macos-archive)
        verify_archive "$ARTIFACTS_DIR" "$PLATFORM"
        exit $?
        ;;
    *)
        echo "❌ Unknown platform: $PLATFORM"
        exit 1
        ;;
esac
