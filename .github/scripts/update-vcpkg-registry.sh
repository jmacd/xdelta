#!/bin/bash
# Script to update vcpkg registry files with new version and SHA512 hash
# Usage: ./update-vcpkg-registry.sh -v VERSION -s SHA512

# Default values
VERSION=""
SHA512=""
DRY_RUN=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -v|--version)
      VERSION="$2"
      shift 2
      ;;
    -s|--sha512)
      SHA512="$2"
      shift 2
      ;;
    --dry-run)
      DRY_RUN=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 -v VERSION -s SHA512 [--dry-run]"
      exit 1
      ;;
  esac
done

# Validate inputs
if [[ -z "$VERSION" ]]; then
  echo "Error: Version is required"
  echo "Usage: $0 -v VERSION -s SHA512 [--dry-run]"
  exit 1
fi

if [[ -z "$SHA512" ]]; then
  echo "Error: SHA512 hash is required"
  echo "Usage: $0 -v VERSION -s SHA512 [--dry-run]"
  exit 1
fi

echo "Updating vcpkg registry for xdelta version $VERSION"
echo "SHA512: $SHA512"
if [[ "$DRY_RUN" == "true" ]]; then
  echo "Running in dry-run mode (no changes will be made)"
fi

# Check if registry files exist
PORTFILE_PATH="vcpkg-registry/ports/xdelta/portfile.cmake"
VCPKG_JSON_PATH="vcpkg-registry/ports/xdelta/vcpkg.json"
VERSION_JSON_PATH="vcpkg-registry/versions/x-/xdelta.json"
BASELINE_JSON_PATH="vcpkg-registry/versions/baseline.json"

if [[ ! -f "$PORTFILE_PATH" ]]; then
  echo "Error: portfile.cmake not found at $PORTFILE_PATH"
  exit 1
fi

if [[ ! -f "$VCPKG_JSON_PATH" ]]; then
  echo "Error: vcpkg.json not found at $VCPKG_JSON_PATH"
  exit 1
fi

if [[ ! -f "$VERSION_JSON_PATH" ]]; then
  echo "Error: xdelta.json not found at $VERSION_JSON_PATH"
  exit 1
fi

if [[ ! -f "$BASELINE_JSON_PATH" ]]; then
  echo "Error: baseline.json not found at $BASELINE_JSON_PATH"
  exit 1
fi

# Get current git commit hash for git-tree
GIT_TREE=$(git rev-parse HEAD)
echo "Current git-tree: $GIT_TREE"

# Update portfile.cmake
if [[ "$DRY_RUN" == "false" ]]; then
  echo "Updating $PORTFILE_PATH..."
  sed -i 's|SHA512 "to-be-filled-after-release"|SHA512 "'"$SHA512"'"|g' "$PORTFILE_PATH"
  sed -i 's|SHA512 "将在发布后填写实际的哈希值"|SHA512 "'"$SHA512"'"|g' "$PORTFILE_PATH"
  sed -i 's|SHA512 "[a-f0-9]*"|SHA512 "'"$SHA512"'"|g' "$PORTFILE_PATH"
  echo "✅ Updated SHA512 in portfile.cmake"
else
  echo "[DRY RUN] Would update SHA512 in $PORTFILE_PATH to: $SHA512"
fi

# Update vcpkg.json
if [[ "$DRY_RUN" == "false" ]]; then
  echo "Updating $VCPKG_JSON_PATH..."
  sed -i 's|"version": "[0-9.]*"|"version": "'"$VERSION"'"|g' "$VCPKG_JSON_PATH"
  echo "✅ Updated version in vcpkg.json"
else
  echo "[DRY RUN] Would update version in $VCPKG_JSON_PATH to: $VERSION"
fi

# Update versions/x-/xdelta.json
if [[ "$DRY_RUN" == "false" ]]; then
  echo "Updating $VERSION_JSON_PATH..."
  sed -i 's|"version": "[0-9.]*"|"version": "'"$VERSION"'"|g' "$VERSION_JSON_PATH"
  sed -i 's|"git-tree": "[a-f0-9]*"|"git-tree": "'"$GIT_TREE"'"|g' "$VERSION_JSON_PATH"
  echo "✅ Updated version and git-tree in xdelta.json"
else
  echo "[DRY RUN] Would update version in $VERSION_JSON_PATH to: $VERSION"
  echo "[DRY RUN] Would update git-tree in $VERSION_JSON_PATH to: $GIT_TREE"
fi

# Update baseline.json
if [[ "$DRY_RUN" == "false" ]]; then
  echo "Updating $BASELINE_JSON_PATH..."
  sed -i 's|"baseline": "[0-9.]*"|"baseline": "'"$VERSION"'"|g' "$BASELINE_JSON_PATH"
  echo "✅ Updated baseline in baseline.json"
else
  echo "[DRY RUN] Would update baseline in $BASELINE_JSON_PATH to: $VERSION"
fi

echo "Registry update complete!"
if [[ "$DRY_RUN" == "true" ]]; then
  echo "This was a dry run. No files were modified."
fi
