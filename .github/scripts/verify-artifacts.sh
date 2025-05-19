#!/bin/bash
# Script to verify artifacts
# Usage: ./verify-artifacts.sh <artifacts_dir> <platform> <arch>

# Check if required arguments are provided
if [ $# -lt 3 ]; then
    echo "Usage: $0 <artifacts_dir> <platform> <arch>"
    echo "  platform: windows, linux, macos"
    echo "  arch: x64, x86 (only relevant for Windows)"
    exit 1
fi

ARTIFACTS_DIR="$1"
PLATFORM="$2"
ARCH="$3"

echo "Verifying artifacts in: $ARTIFACTS_DIR"
echo "Platform: $PLATFORM"
echo "Architecture: $ARCH"

# Check if artifacts directory exists
if [ ! -d "$ARTIFACTS_DIR" ]; then
    echo "❌ Artifacts directory not found: $ARTIFACTS_DIR"
    exit 1
fi

# List all files in the artifacts directory
echo "Files in artifacts directory:"
ls -la "$ARTIFACTS_DIR"

# Function to verify Windows artifacts
verify_windows_artifacts() {
    local arch="$1"
    local dir="$ARTIFACTS_DIR"
    
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
    
    echo "✅ All required Windows $arch files found"
    return 0
}

# Function to verify Linux artifacts
verify_linux_artifacts() {
    local dir="$ARTIFACTS_DIR"
    
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
    local dir="$ARTIFACTS_DIR"
    
    # Check for README.txt (macOS build is disabled, so only README is required)
    if [ ! -f "$dir/README.txt" ]; then
        echo "❌ README.txt not found in $dir"
        return 1
    fi
    
    echo "✅ All required macOS files found (note: macOS build is disabled)"
    return 0
}

# Verify artifacts based on platform
case "$PLATFORM" in
    windows)
        verify_windows_artifacts "$ARCH"
        exit $?
        ;;
    linux)
        verify_linux_artifacts
        exit $?
        ;;
    macos)
        verify_macos_artifacts
        exit $?
        ;;
    *)
        echo "❌ Unknown platform: $PLATFORM"
        exit 1
        ;;
esac
