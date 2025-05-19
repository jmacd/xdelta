#!/bin/bash
# Script to create README.txt for artifacts
# Usage: ./create-readme.sh <artifacts_dir> <platform> [<version>]

# Check if required arguments are provided
if [ $# -lt 2 ]; then
    echo "Usage: $0 <artifacts_dir> <platform> [<version>]"
    echo "  platform: windows, linux, macos"
    echo "  version: optional version number"
    exit 1
fi

ARTIFACTS_DIR="$1"
PLATFORM="$2"
VERSION="${3:-3.1.0}"  # Default version if not provided

# Ensure artifacts directory exists
mkdir -p "$ARTIFACTS_DIR"

# Create README path
README_PATH="$ARTIFACTS_DIR/README.txt"

echo "Creating README file for $PLATFORM at: $README_PATH"

# Create platform-specific README content
case "$PLATFORM" in
    windows)
        cat > "$README_PATH" << EOF
Xdelta3 Windows Binary
======================

This package contains the xdelta3 command-line utility for Windows.

Command Line Syntax
-------------------

make patch:

  xdelta3.exe -e -s old_file new_file delta_file

apply patch:

  xdelta3.exe -d -s old_file delta_file decoded_new_file

standard options:
   -0 .. -9     compression level
   -c           use stdout
   -d           decompress
   -e           compress
   -f           force (overwrite, ignore trailing garbage)
   -h           show help
   -q           be quiet
   -v           be verbose (max 2)
   -V           show version

For full documentation, run: xdelta3.exe --help
EOF
        ;;
    linux)
        cat > "$README_PATH" << EOF
Xdelta3 Linux Binary
===================

This package contains the xdelta3 command-line utility for Linux.

Command Line Syntax
-------------------

make patch:

  xdelta3 -e -s old_file new_file delta_file

apply patch:

  xdelta3 -d -s old_file delta_file decoded_new_file

standard options:
   -0 .. -9     compression level
   -c           use stdout
   -d           decompress
   -e           compress
   -f           force (overwrite, ignore trailing garbage)
   -h           show help
   -q           be quiet
   -v           be verbose (max 2)
   -V           show version

For full documentation, run: xdelta3 --help
EOF
        ;;
    macos)
        cat > "$README_PATH" << EOF
Xdelta3 macOS Binary (Temporarily Disabled)
===========================================

NOTICE: macOS build is temporarily disabled due to C++11 literal suffix issues.
Please use Windows or Linux platforms.

For more information, see the project repository.
EOF
        ;;
    *)
        echo "ERROR: Unknown platform: $PLATFORM"
        exit 1
        ;;
esac

# Verify README was created successfully
if [ -f "$README_PATH" ]; then
    echo "README.txt created successfully"
    head -n 3 "$README_PATH"
else
    echo "ERROR: Failed to create README.txt"
    exit 1
fi
