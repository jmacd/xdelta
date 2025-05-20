#!/bin/bash
# 创建指定平台的占位符构建产物
# 用法: ./create-placeholder.sh <platform> <artifacts_dir> <output_dir> [<arch>]

set -e

PLATFORM="$1"
ARTIFACTS_DIR="$2"
OUTPUT_DIR="$3"
ARCH="${4:-x64}"

echo "Creating placeholder for $PLATFORM"

# 根据平台创建占位符
case "$PLATFORM" in
    windows)
        mkdir -p "$ARTIFACTS_DIR"
        echo "This is a placeholder file for testing" > "$ARTIFACTS_DIR/xdelta3.exe"
        $(dirname "$0")/create-readme.sh "$ARTIFACTS_DIR" "$PLATFORM"
        
        # 创建zip包
        mkdir -p "$OUTPUT_DIR"
        zip -j "$OUTPUT_DIR/xdelta3-$PLATFORM-$ARCH.zip" "$ARTIFACTS_DIR/xdelta3.exe" "$ARTIFACTS_DIR/README.txt"
        echo "✅ Created Windows $ARCH placeholder"
        ;;
    linux)
        mkdir -p "$ARTIFACTS_DIR"
        echo "This is a placeholder file for testing" > "$ARTIFACTS_DIR/xdelta3"
        chmod +x "$ARTIFACTS_DIR/xdelta3"
        $(dirname "$0")/create-readme.sh "$ARTIFACTS_DIR" "$PLATFORM"
        
        # 创建tar.gz包
        mkdir -p "$OUTPUT_DIR"
        tar --warning=no-file-changed -czf "$OUTPUT_DIR/xdelta3-$PLATFORM.tar.gz" -C "$ARTIFACTS_DIR" .
        echo "✅ Created Linux placeholder"
        ;;
    macos)
        mkdir -p "$ARTIFACTS_DIR"
        $(dirname "$0")/create-readme.sh "$ARTIFACTS_DIR" "$PLATFORM"
        
        # 创建tar.gz包
        mkdir -p "$OUTPUT_DIR"
        tar --warning=no-file-changed -czf "$OUTPUT_DIR/xdelta3-$PLATFORM.tar.gz" -C "$ARTIFACTS_DIR" .
        echo "✅ Created macOS placeholder"
        ;;
    *)
        echo "❌ Unknown platform: $PLATFORM"
        exit 1
        ;;
esac

exit 0
