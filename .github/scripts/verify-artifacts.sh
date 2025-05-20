#!/bin/bash
# 验证指定平台的构建产物
# 用法: ./verify-artifacts.sh <artifacts_dir> <platform> [<arch>]

set -e

ARTIFACTS_DIR="$1"
PLATFORM="$2"
ARCH="${3:-x64}"

echo "Verifying artifacts for $PLATFORM in $ARTIFACTS_DIR"

# 检查构建产物目录是否存在
if [ ! -d "$ARTIFACTS_DIR" ]; then
    echo "❌ Artifacts directory not found: $ARTIFACTS_DIR"
    exit 1
fi

# 列出构建产物目录中的所有文件
echo "Files in artifacts directory:"
ls -la "$ARTIFACTS_DIR"

# 根据平台验证
case "$PLATFORM" in
    windows)
        # 检查 xdelta3.exe
        if [ ! -f "$ARTIFACTS_DIR/xdelta3.exe" ]; then
            echo "❌ xdelta3.exe not found in $ARTIFACTS_DIR"
            exit 1
        fi
        
        # 检查 README.txt
        if [ ! -f "$ARTIFACTS_DIR/README.txt" ]; then
            echo "❌ README.txt not found in $ARTIFACTS_DIR"
            exit 1
        fi
        
        echo "✅ All required Windows $ARCH files found"
        ;;
    linux)
        # 检查 xdelta3
        if [ ! -f "$ARTIFACTS_DIR/xdelta3" ]; then
            echo "❌ xdelta3 not found in $ARTIFACTS_DIR"
            exit 1
        fi
        
        # 检查 README.txt
        if [ ! -f "$ARTIFACTS_DIR/README.txt" ]; then
            echo "❌ README.txt not found in $ARTIFACTS_DIR"
            exit 1
        fi
        
        echo "✅ All required Linux files found"
        ;;
    macos)
        # 检查 README.txt (macOS构建已禁用，所以只需要README)
        if [ ! -f "$ARTIFACTS_DIR/README.txt" ]; then
            echo "❌ README.txt not found in $ARTIFACTS_DIR"
            exit 1
        fi
        
        echo "✅ All required macOS files found (note: macOS build is disabled)"
        ;;
    *)
        echo "❌ Unknown platform: $PLATFORM"
        exit 1
        ;;
esac

exit 0
