#!/bin/bash
# Script to create README.txt for macOS artifacts
# Usage: ./create-readme-macos.sh <artifacts_dir>

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
Xdelta3 macOS Binary (Temporarily Disabled)
===========================================

NOTICE: macOS build is temporarily disabled due to C++11 literal suffix issues.
Please use Windows or Linux platforms.

For more information, see the project repository.
EOF

# Verify README was created successfully
if [ -f "$README_PATH" ]; then
    echo "README.txt created successfully"
    head -n 3 "$README_PATH"
else
    echo "ERROR: Failed to create README.txt"
    exit 1
fi
