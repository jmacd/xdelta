#!/bin/bash
# Script to verify vcpkg registry files are correctly updated
# Usage: ./verify-vcpkg-registry.sh -v VERSION -s SHA512

# Default values
VERSION=""
SHA512=""

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
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 -v VERSION -s SHA512"
      exit 1
      ;;
  esac
done

# Validate inputs
if [[ -z "$VERSION" ]]; then
  echo "Error: Version is required"
  echo "Usage: $0 -v VERSION -s SHA512"
  exit 1
fi

if [[ -z "$SHA512" ]]; then
  echo "Error: SHA512 hash is required"
  echo "Usage: $0 -v VERSION -s SHA512"
  exit 1
fi

echo "Verifying vcpkg registry for xdelta version $VERSION"
echo "Expected SHA512: $SHA512"

# Check if registry files exist
PORTFILE_PATH="vcpkg-registry/ports/xdelta/portfile.cmake"
VCPKG_JSON_PATH="vcpkg-registry/ports/xdelta/vcpkg.json"
VERSION_JSON_PATH="vcpkg-registry/versions/x-/xdelta.json"
BASELINE_JSON_PATH="vcpkg-registry/versions/baseline.json"

if [[ ! -f "$PORTFILE_PATH" ]]; then
  echo "❌ Error: portfile.cmake not found at $PORTFILE_PATH"
  exit 1
fi

if [[ ! -f "$VCPKG_JSON_PATH" ]]; then
  echo "❌ Error: vcpkg.json not found at $VCPKG_JSON_PATH"
  exit 1
fi

if [[ ! -f "$VERSION_JSON_PATH" ]]; then
  echo "❌ Error: xdelta.json not found at $VERSION_JSON_PATH"
  exit 1
fi

if [[ ! -f "$BASELINE_JSON_PATH" ]]; then
  echo "❌ Error: baseline.json not found at $BASELINE_JSON_PATH"
  exit 1
fi

# Initialize verification status
VERIFICATION_FAILED=0

# Verify portfile.cmake
echo "Checking portfile.cmake SHA512..."
PORTFILE_CONTENT=$(cat "$PORTFILE_PATH")
echo "portfile.cmake content (excerpt):"
grep -n "SHA512" "$PORTFILE_PATH" || echo "SHA512 not found in portfile.cmake"

# Try different patterns to extract SHA512
PORTFILE_SHA512=$(grep -oP 'SHA512 "\K[a-f0-9]+(?=")' "$PORTFILE_PATH" || echo "")
if [[ -z "$PORTFILE_SHA512" ]]; then
  # Try alternative pattern
  PORTFILE_SHA512=$(grep -o 'SHA512 "[a-f0-9]*"' "$PORTFILE_PATH" | grep -o '"[a-f0-9]*"' | tr -d '"' || echo "")
fi

if [[ "$PORTFILE_SHA512" == "$SHA512" ]]; then
  echo "✅ portfile.cmake SHA512 is correct"
else
  echo "❌ portfile.cmake SHA512 is incorrect"
  echo "  Expected: $SHA512"
  echo "  Found:    $PORTFILE_SHA512"
  # Don't exit immediately, continue checking other files
  VERIFICATION_FAILED=1
fi

# Verify vcpkg.json
echo "Checking vcpkg.json version..."
grep -n "version" "$VCPKG_JSON_PATH" || echo "version not found in vcpkg.json"

VCPKG_JSON_VERSION=$(grep -oP '"version": "\K[0-9.]+(?=")' "$VCPKG_JSON_PATH" || echo "")
if [[ "$VCPKG_JSON_VERSION" == "$VERSION" ]]; then
  echo "✅ vcpkg.json version is correct"
else
  echo "❌ vcpkg.json version is incorrect"
  echo "  Expected: $VERSION"
  echo "  Found:    $VCPKG_JSON_VERSION"
  VERIFICATION_FAILED=1
fi

# Verify versions/x-/xdelta.json
echo "Checking xdelta.json version..."
grep -n "version" "$VERSION_JSON_PATH" || echo "version not found in xdelta.json"

VERSION_JSON_VERSION=$(grep -oP '"version": "\K[0-9.]+(?=")' "$VERSION_JSON_PATH" || echo "")
if [[ "$VERSION_JSON_VERSION" == "$VERSION" ]]; then
  echo "✅ xdelta.json version is correct"
else
  echo "❌ xdelta.json version is incorrect"
  echo "  Expected: $VERSION"
  echo "  Found:    $VERSION_JSON_VERSION"
  VERIFICATION_FAILED=1
fi

# Verify git-tree exists
echo "Checking xdelta.json git-tree..."
grep -n "git-tree" "$VERSION_JSON_PATH" || echo "git-tree not found in xdelta.json"

if grep -q '"git-tree":' "$VERSION_JSON_PATH"; then
  echo "✅ xdelta.json git-tree exists"
else
  echo "❌ xdelta.json git-tree is missing"
  VERIFICATION_FAILED=1
fi

# Verify baseline.json
echo "Checking baseline.json version..."
grep -n "baseline" "$BASELINE_JSON_PATH" || echo "baseline not found in baseline.json"

BASELINE_VERSION=$(grep -oP '"baseline": "\K[0-9.]+(?=")' "$BASELINE_JSON_PATH" || echo "")
if [[ "$BASELINE_VERSION" == "$VERSION" ]]; then
  echo "✅ baseline.json version is correct"
else
  echo "❌ baseline.json version is incorrect"
  echo "  Expected: $VERSION"
  echo "  Found:    $BASELINE_VERSION"
  VERIFICATION_FAILED=1
fi

# Check final verification status
if [[ $VERIFICATION_FAILED -eq 0 ]]; then
  echo "✅ All vcpkg registry files are correctly updated!"
  exit 0
else
  echo "❌ Some vcpkg registry files have issues. See details above."
  exit 1
fi
