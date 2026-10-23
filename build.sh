#!/bin/bash
# build.sh - Build BLACK Browser on Linux / Codespaces

set -e

CONFIG="Release"
BUILD_DIR="build"

echo "=== BLACK Browser Build Script (Linux) ==="

# Find Qt6
if [ -z "$QT_ROOT" ]; then
    if [ -d "/opt/Qt/6.7.2/gcc_64" ]; then
        export QT_ROOT="/opt/Qt/6.7.2/gcc_64"
    elif [ -d "/usr/lib/qt6" ]; then
        export QT_ROOT="/usr/lib/qt6"
    else
        echo "❌ Qt6 not found. Set QT_ROOT or install Qt6."
        exit 1
    fi
fi

echo "Using Qt at: $QT_ROOT"

# Configure
echo "Configuring with CMake..."
cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_PREFIX_PATH="$QT_ROOT" -DCMAKE_BUILD_TYPE="$CONFIG"

# Build
echo "Building ($CONFIG)..."
cmake --build "$BUILD_DIR" --parallel

# Verify
EXE="$BUILD_DIR/BLACK"
if [ -f "$EXE" ]; then
    echo ""
    echo "✅ Success! Executable at: $(realpath $EXE)"
    echo "Run it: $EXE"
else
    echo "❌ Build completed but executable not found at $EXE"
    exit 1
fi