#!/bin/bash
# Unified script for artifact handling (create, verify, package)
# This script replaces the separate process-artifacts.sh and verify-artifacts.sh scripts
# Usage: ./unified-artifacts.sh <command> <platform> <artifacts_dir> [<arch>] [<output_file>]

set -e  # Exit on error

# Check if required arguments are provided
if [ $# -lt 3 ]; then
    echo "Usage: $0 <command> <platform> <artifacts_dir> [<arch>] [<output_file>]"
    echo "  command: create, verify, package, extract"
    echo "  platform: windows, linux, macos"
    echo "  artifacts_dir: directory containing artifacts or to create artifacts in"
    echo "  arch: x64, x86 (only relevant for Windows, defaults to x64)"
    echo "  output_file: output file for package command (required for package command)"
    exit 1
fi

COMMAND="$1"
PLATFORM="$2"
ARTIFACTS_DIR="$3"
ARCH="${4:-x64}"  # Default to x64 if not provided
OUTPUT_FILE="$5"

# Ensure scripts directory is available
SCRIPTS_DIR="$(dirname "$0")"

# Make scripts executable
chmod +x "$SCRIPTS_DIR"/*.sh

# Define standard artifact paths
get_artifact_path() {
    local platform="$1"
    local arch="$2"
    
    case "$platform" in
        windows)
            echo "artifacts/windows-$arch"
            ;;
        linux)
            echo "artifacts/linux"
            ;;
        macos)
            echo "artifacts/macos"
            ;;
        *)
            echo "ERROR: Unknown platform: $platform" >&2
            exit 1
            ;;
    esac
}

# Define standard archive paths
get_archive_path() {
    local platform="$1"
    local arch="$2"
    
    case "$platform" in
        windows)
            echo "xdelta3-windows-$arch.zip"
            ;;
        linux)
            echo "xdelta3-linux.tar.gz"
            ;;
        macos)
            echo "xdelta3-macos.tar.gz"
            ;;
        *)
            echo "ERROR: Unknown platform: $platform" >&2
            exit 1
            ;;
    esac
}

# Function to create artifacts
create_artifacts() {
    local platform="$1"
    local artifacts_dir="$2"
    local arch="$3"
    
    echo "Creating artifacts for $platform in $artifacts_dir"
    
    # Ensure directory exists
    mkdir -p "$artifacts_dir"
    
    # Create README
    create_readme "$artifacts_dir" "$platform"
    
    echo "Artifacts created successfully"
    return 0
}

# Function to create README
create_readme() {
    local artifacts_dir="$1"
    local platform="$2"
    local version="3.1.0"  # Default version
    
    # Create README path
    local readme_path="$artifacts_dir/README.txt"
    
    echo "Creating README file for $platform at: $readme_path"
    
    # Create platform-specific README content
    case "$platform" in
        windows)
            cat > "$readme_path" << EOF
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
            cat > "$readme_path" << EOF
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
            cat > "$readme_path" << EOF
Xdelta3 macOS Binary (Temporarily Disabled)
===========================================

NOTICE: macOS build is temporarily disabled due to C++11 literal suffix issues.
Please use Windows or Linux platforms.

For more information, see the project repository.
EOF
            ;;
        *)
            echo "ERROR: Unknown platform: $platform"
            exit 1
            ;;
    esac
    
    # Verify README was created successfully
    if [ -f "$readme_path" ]; then
        echo "README.txt created successfully"
        head -n 3 "$readme_path"
    else
        echo "ERROR: Failed to create README.txt"
        exit 1
    fi
}

# Function to verify artifacts
verify_artifacts() {
    local platform="$1"
    local artifacts_dir="$2"
    local arch="$3"
    
    echo "Verifying artifacts for $platform in $artifacts_dir"
    
    # Check if artifacts directory exists
    if [ ! -d "$artifacts_dir" ]; then
        echo "❌ Artifacts directory not found: $artifacts_dir"
        return 1
    fi
    
    # List all files in the artifacts directory
    echo "Files in artifacts directory:"
    ls -la "$artifacts_dir"
    
    # Verify based on platform
    case "$platform" in
        windows)
            verify_windows_artifacts "$artifacts_dir" "$arch"
            return $?
            ;;
        linux)
            verify_linux_artifacts "$artifacts_dir"
            return $?
            ;;
        macos)
            verify_macos_artifacts "$artifacts_dir"
            return $?
            ;;
        *)
            echo "❌ Unknown platform: $platform"
            return 1
            ;;
    esac
}

# Function to verify Windows artifacts
verify_windows_artifacts() {
    local dir="$1"
    local arch="$2"
    
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
    
    # Check for liblzma.dll (optional)
    if [ ! -f "$dir/liblzma.dll" ]; then
        echo "⚠️ liblzma.dll not found in $dir (may be statically linked)"
    fi
    
    echo "✅ All required Windows $arch files found"
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

# Function to package artifacts
package_artifacts() {
    local platform="$1"
    local artifacts_dir="$2"
    local arch="$3"
    local output_file="$4"
    
    if [ -z "$output_file" ]; then
        echo "ERROR: Output file is required for package command"
        return 1
    fi
    
    echo "Packaging artifacts for $platform from $artifacts_dir to $output_file"
    
    # Verify artifacts first
    verify_artifacts "$platform" "$artifacts_dir" "$arch"
    if [ $? -ne 0 ]; then
        echo "ERROR: Artifact verification failed, aborting packaging"
        return 1
    fi
    
    # Package based on platform
    case "$platform" in
        windows)
            # Create zip archive
            zip -j "$output_file" "$artifacts_dir"/*
            ;;
        linux|macos)
            # Create tar.gz archive
            tar -czf "$output_file" -C "$artifacts_dir" .
            ;;
        *)
            echo "ERROR: Unknown platform: $platform"
            return 1
            ;;
    esac
    
    # Verify the package was created
    if [ -f "$output_file" ]; then
        echo "Package created successfully: $output_file"
        # Copy the archive to artifacts directory for consistency
        cp "$output_file" "$artifacts_dir/"
        echo "Copied archive to artifacts directory: $artifacts_dir/$(basename "$output_file")"
        return 0
    else
        echo "ERROR: Failed to create package: $output_file"
        return 1
    fi
}

# Function to extract archive
extract_archive() {
    local archive_path="$1"
    local output_dir="$2"
    local platform="$3"
    
    echo "Extracting archive $archive_path to $output_dir"
    
    # Create output directory if it doesn't exist
    mkdir -p "$output_dir"
    
    # Extract based on archive type
    if [[ "$archive_path" == *.zip ]]; then
        unzip -o "$archive_path" -d "$output_dir"
    elif [[ "$archive_path" == *.tar.gz ]]; then
        tar -xzf "$archive_path" -C "$output_dir"
    else
        echo "❌ Unknown archive format: $archive_path"
        return 1
    fi
    
    echo "Archive extracted successfully"
    return 0
}

# Execute the requested command
case "$COMMAND" in
    create)
        create_artifacts "$PLATFORM" "$ARTIFACTS_DIR" "$ARCH"
        exit $?
        ;;
    verify)
        verify_artifacts "$PLATFORM" "$ARTIFACTS_DIR" "$ARCH"
        exit $?
        ;;
    package)
        package_artifacts "$PLATFORM" "$ARTIFACTS_DIR" "$ARCH" "$OUTPUT_FILE"
        exit $?
        ;;
    extract)
        extract_archive "$ARTIFACTS_DIR" "$OUTPUT_FILE" "$PLATFORM"
        exit $?
        ;;
    *)
        echo "ERROR: Unknown command: $COMMAND"
        exit 1
        ;;
esac
