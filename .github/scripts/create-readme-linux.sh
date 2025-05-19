#!/bin/bash
# Script to create README.txt for Linux artifacts
# Usage: ./create-readme-linux.sh <artifacts_dir>

# Check if artifacts directory is provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <artifacts_dir>"
    exit 1
fi

ARTIFACTS_DIR="$1"

# Ensure artifacts directory exists
mkdir -p "$ARTIFACTS_DIR"

# Create README path
README_PATH="$ARTIFACTS_DIR/README.txt"

echo "Creating README file at: $README_PATH"

# Create the README content
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

# Verify README was created successfully
if [ -f "$README_PATH" ]; then
    echo "README.txt created successfully"
    head -n 3 "$README_PATH"
else
    echo "ERROR: Failed to create README.txt"
    exit 1
fi
