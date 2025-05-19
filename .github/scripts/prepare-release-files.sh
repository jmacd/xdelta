#!/bin/bash
# Script to prepare release files from artifacts
# Usage: ./prepare-release-files.sh <version> <output_dir>

# Check if required arguments are provided
if [ $# -lt 2 ]; then
    echo "Usage: $0 <version> <output_dir>"
    exit 1
fi

VERSION="$1"
OUTPUT_DIR="$2"

echo "Preparing release files for version $VERSION..."

# Create a directory for all release files
mkdir -p "$OUTPUT_DIR"

# Function to handle Windows artifacts
handle_windows_artifacts() {
    ARCH="$1"
    
    echo "Handling Windows $ARCH artifacts..."
    
    # Check for zip file in various locations
    if [ -f "xdelta3-windows-$ARCH.zip" ]; then
        echo "✅ Found Windows $ARCH zip artifact"
        cp "xdelta3-windows-$ARCH.zip" "$OUTPUT_DIR/"
        return 0
    elif [ -f "windows-$ARCH-zip/xdelta3-windows-$ARCH.zip" ]; then
        echo "✅ Found Windows $ARCH zip artifact in windows-$ARCH-zip directory"
        cp "windows-$ARCH-zip/xdelta3-windows-$ARCH.zip" "$OUTPUT_DIR/"
        return 0
    elif [ -d "windows-$ARCH" ]; then
        echo "Found Windows $ARCH directory artifact"
        zip -r "$OUTPUT_DIR/xdelta3-windows-$ARCH.zip" "windows-$ARCH/"
        return 0
    elif [ -d "windows-$ARCH-artifacts" ]; then
        echo "Found Windows $ARCH directory artifact in windows-$ARCH-artifacts"
        zip -r "$OUTPUT_DIR/xdelta3-windows-$ARCH.zip" "windows-$ARCH-artifacts/"
        return 0
    else
        echo "⚠️ Windows $ARCH artifact not found in expected locations, checking all directories"
        # List all directories to help debug
        find . -type d -maxdepth 1 | sort
        
        # Try to find any zip file that might contain the Windows artifacts
        FOUND_ZIP=$(find . -name "*$ARCH*.zip" -type f | head -1)
        if [ -n "$FOUND_ZIP" ]; then
            echo "Found potential Windows $ARCH zip: $FOUND_ZIP"
            cp "$FOUND_ZIP" "$OUTPUT_DIR/xdelta3-windows-$ARCH.zip"
            return 0
        else
            echo "❌ Windows $ARCH artifact not found"
            return 1
        fi
    fi
}

# Function to handle Linux artifacts
handle_linux_artifacts() {
    echo "Handling Linux artifacts..."
    
    # Check for tar.gz file in various locations
    if [ -f "xdelta3-linux.tar.gz" ]; then
        echo "✅ Found Linux tar artifact"
        cp "xdelta3-linux.tar.gz" "$OUTPUT_DIR/"
        return 0
    elif [ -f "xdelta3-linux-tar.tar.gz" ]; then
        echo "✅ Found Linux tar artifact with different name"
        cp "xdelta3-linux-tar.tar.gz" "$OUTPUT_DIR/xdelta3-linux.tar.gz"
        return 0
    elif [ -d "linux" ]; then
        echo "Found Linux directory artifact"
        tar -czf "$OUTPUT_DIR/xdelta3-linux.tar.gz" -C "linux" .
        return 0
    else
        echo "⚠️ Linux artifact not found in expected locations, checking all directories"
        # Try to find any tar.gz file that might contain the Linux artifacts
        FOUND_TAR=$(find . -name "*linux*.tar.gz" -type f | head -1)
        if [ -n "$FOUND_TAR" ]; then
            echo "Found potential Linux tar: $FOUND_TAR"
            cp "$FOUND_TAR" "$OUTPUT_DIR/xdelta3-linux.tar.gz"
            return 0
        else
            echo "❌ Linux artifact not found"
            return 1
        fi
    fi
}

# Function to handle macOS artifacts
handle_macos_artifacts() {
    echo "Handling macOS artifacts..."
    
    # Check for tar.gz file in various locations
    if [ -f "xdelta3-macos.tar.gz" ]; then
        echo "✅ Found macOS tar artifact"
        cp "xdelta3-macos.tar.gz" "$OUTPUT_DIR/"
        return 0
    elif [ -f "xdelta3-macos-tar.tar.gz" ]; then
        echo "✅ Found macOS tar artifact with different name"
        cp "xdelta3-macos-tar.tar.gz" "$OUTPUT_DIR/xdelta3-macos.tar.gz"
        return 0
    elif [ -d "macos" ]; then
        echo "Found macOS directory artifact"
        tar -czf "$OUTPUT_DIR/xdelta3-macos.tar.gz" -C "macos" .
        return 0
    else
        echo "⚠️ macOS artifact not found (this is expected)"
        # Create an empty file as a placeholder
        echo "This is a placeholder for macOS binaries which are currently not supported." > "$OUTPUT_DIR/xdelta3-macos.tar.gz"
        return 0
    fi
}

# Handle Windows x64 artifacts
handle_windows_artifacts "x64" || exit 1

# Handle Windows x86 artifacts
handle_windows_artifacts "x86" || exit 1

# Handle Linux artifacts
handle_linux_artifacts || exit 1

# Handle macOS artifacts
handle_macos_artifacts

echo "All release files prepared successfully in $OUTPUT_DIR"
ls -la "$OUTPUT_DIR"
