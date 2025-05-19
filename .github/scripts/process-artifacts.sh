#!/bin/bash
# Script to process artifacts (create, verify, package)
# Usage: ./process-artifacts.sh <command> <platform> <artifacts_dir> [<arch>] [<output_file>]

# Check if required arguments are provided
if [ $# -lt 3 ]; then
    echo "Usage: $0 <command> <platform> <artifacts_dir> [<arch>] [<output_file>]"
    echo "  command: create, verify, package"
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

# Function to create artifacts
create_artifacts() {
    local platform="$1"
    local artifacts_dir="$2"
    local arch="$3"
    
    echo "Creating artifacts for $platform in $artifacts_dir"
    
    # Create README
    "$SCRIPTS_DIR/create-readme.sh" "$artifacts_dir" "$platform"
    
    # Additional platform-specific setup could be added here
    
    echo "Artifacts created successfully"
    return 0
}

# Function to verify artifacts
verify_artifacts() {
    local platform="$1"
    local artifacts_dir="$2"
    local arch="$3"
    
    echo "Verifying artifacts for $platform in $artifacts_dir"
    
    # Verify artifacts
    "$SCRIPTS_DIR/verify-artifacts.sh" "$artifacts_dir" "$platform" "$arch"
    return $?
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
            zip -r "$output_file" "$artifacts_dir"/*
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
        return 0
    else
        echo "ERROR: Failed to create package: $output_file"
        return 1
    fi
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
    *)
        echo "ERROR: Unknown command: $COMMAND"
        exit 1
        ;;
esac
