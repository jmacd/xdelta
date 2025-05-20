#!/bin/bash
# 打包指定平台的构建产物
# 用法: ./package-artifacts.sh <platform> <artifacts_dir> <output_file> [<arch>]

set -e

PLATFORM="$1"
ARTIFACTS_DIR="$2"
OUTPUT_FILE="$3"
ARCH="${4:-x64}"

echo "Packaging artifacts for $PLATFORM from $ARTIFACTS_DIR to $OUTPUT_FILE"

# 验证构建产物
$(dirname "$0")/verify-artifacts.sh "$ARTIFACTS_DIR" "$PLATFORM" "$ARCH"
if [ $? -ne 0 ]; then
    echo "ERROR: Artifact verification failed, aborting packaging"
    exit 1
fi

# 根据平台打包
case "$PLATFORM" in
    windows)
        # 创建zip归档
        zip -j "$OUTPUT_FILE" "$ARTIFACTS_DIR"/*
        ;;
    linux|macos)
        # 创建tar.gz归档 - 使用--warning=no-file-changed避免"file changed as we read it"错误
        tar --warning=no-file-changed -czf "$OUTPUT_FILE" -C "$ARTIFACTS_DIR" .
        ;;
    *)
        echo "ERROR: Unknown platform: $PLATFORM"
        exit 1
        ;;
esac

# 验证包是否创建成功
if [ -f "$OUTPUT_FILE" ]; then
    echo "Package created successfully: $OUTPUT_FILE"
    exit 0
else
    echo "ERROR: Failed to create package: $OUTPUT_FILE"
    exit 1
fi
