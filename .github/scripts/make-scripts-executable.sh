#!/bin/bash
# Script to make all scripts in .github/scripts directory executable
# Usage: ./make-scripts-executable.sh

SCRIPTS_DIR="$(dirname "$0")"
echo "Making all scripts in $SCRIPTS_DIR executable..."

# Find all shell scripts and make them executable
find "$SCRIPTS_DIR" -name "*.sh" -type f -exec chmod +x {} \;
echo "Made all shell scripts executable"

# List all scripts with their permissions
echo "Script permissions:"
ls -la "$SCRIPTS_DIR"
