#!/bin/bash

# Check if PowerShell is installed
if command -v pwsh &> /dev/null; then
    POWERSHELL_CMD="pwsh"
elif command -v powershell &> /dev/null; then
    POWERSHELL_CMD="powershell"
else
    echo "PowerShell is not installed. Please install PowerShell Core (pwsh) to run this script."
    echo "Visit: https://github.com/PowerShell/PowerShell#get-powershell"
    exit 1
fi

# Get the directory of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo "Running format string fix script for all C/C++ files..."
$POWERSHELL_CMD -ExecutionPolicy Bypass -File "$SCRIPT_DIR/fix_format_strings.ps1"

echo "Script completed."
