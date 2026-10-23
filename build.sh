#!/bin/bash
# build.sh - Build BLACK Browser on Linux / Codespaces

set -e

CONFIG="Release"
BUILD_DIR="build"

echo "=== BLACK Browser Build Script (Linux) ==="

# Auto-detect Qt6
if [ -n "$QT_ROOT" ] && [ -f "$QT_ROOT/bin/qmake" ]; then
    echo "Using QT_ROOT from environment: $QT_ROOT"
elif [ -d "/opt/Qt/6.7.2/gcc_64" ] && [ -f "/opt/Qt/6.7.2/gcc_64/bin/qmake" ]; then
    export QT_ROOT="/opt/Qt/6.7.2/gcc_64"
    echo "Found Qt at /opt/Qt/6.7.2/gcc_64"
elif [ -d "/usr/lib/qt6" ] && [ -f "/usr/lib/qt6/bin/qmake" ]; then
    export QT_ROOT="/usr/lib/qt6"
    echo "Found Qt at /usr/lib/qt6 (system package)"
elif command -v qmake6 >/dev/null 2>&1; then
    export QT_ROOT="$(dirname $(dirname $(which qmake6)))"
    echo "Found Qt via qmake6: $QT_ROOT"
else
    echo "❌ Qt6 not found. Tried:"
    echo "  - QT_ROOT env var"
    echo "  - /opt/Qt/6.7.2/gcc_64"
    echo "  - /usr/lib/qt6"
    echo "  - qmake6 in PATH"
    echo ""
    echo "In Codespaces: run 'source ~/.bashrc' after postCreateCommand completes"
    echo "Or reopen terminal (Ctrl+Shift+`)"
    exit 1
fi

echo "Using Qt at: $QT_ROOT"

# Verify WebEngine
if [ ! -f "$QT_ROOT/lib/cmake/Qt6WebEngineWidgets/Qt6WebEngineWidgetsConfig.cmake" ] && \
   [ ! -f "$QT_ROOT/lib/cmake/Qt6WebEngineWidgetsConfig.cmake" ]; then
    echo "⚠️  Warning: QtWebEngineWidgets not found at $QT_ROOT"
    echo "   Build may fail. Ensure qt6-webengine-dev is installed."
fi

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
    echo ""
    echo "To download from Codespaces:"
    echo "  1. Right-click 'build/BLACK' in file explorer"
    echo "  2. Select 'Download'"
else
    echo "❌ Build completed but executable not found at $EXE"
    exit 1
fi