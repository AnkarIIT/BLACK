#!/bin/bash
# .devcontainer/setup-qt.sh - Install Qt6 in Codespaces (robust version)

set -e

echo "=== Installing Qt 6 for BLACK Browser ==="

# Method 1: Try apt first (fastest, works in Codespaces with full internet)
echo "Trying apt packages (Qt 6.6 + WebEngine)..."
if sudo apt-get update && sudo apt-get install -y \
    qt6-base-dev qt6-webengine-dev qt6-svg-dev \
    libssl-dev libsecret-1-dev \
    cmake ninja-build g++ 2>/dev/null; then
    
    # Check if WebEngine is actually available
    if pkg-config --exists Qt6WebEngineWidgets 2>/dev/null; then
        echo "=== Qt installed via apt (with WebEngine) ==="
        export QT_ROOT="/usr/lib/qt6"
        echo 'export QT_ROOT="/usr/lib/qt6"' >> ~/.bashrc
        echo 'export PATH="/usr/lib/qt6/bin:$PATH"' >> ~/.bashrc
        exit 0
    fi
    echo "Qt installed but WebEngine missing, falling back to official installer..."
fi

# Method 2: Official Qt installer (guaranteed WebEngine 6.7.2)
echo "Installing Qt 6.7.2 via official installer..."
QT_VERSION="6.7.2"
QT_INSTALLER="qt-unified-linux-x64-online.run"
QT_URL="https://download.qt.io/official_releases/online_installers/${QT_INSTALLER}"

cd /tmp
wget -q "${QT_URL}" -O "${QT_INSTALLER}"
chmod +x "${QT_INSTALLER}"

# Create non-interactive install script
cat > /tmp/qt-install-script.qs <<'QTSCRIPT'
Component.installationPrefix = '/opt/Qt';
Component.autoAnswer = true;
Component.installComponents = ['qt.qt6.672.gcc_64', 'qt.qt6.672.qtwebengine.gcc_64', 'qt.qt6.672.qtsvg.gcc_64'];
QTSCRIPT

./"${QT_INSTALLER}" --script /tmp/qt-install-script.qs --verbose

# Add to PATH
echo 'export PATH="/opt/Qt/6.7.2/gcc_64/bin:$PATH"' >> ~/.bashrc
export PATH="/opt/Qt/6.7.2/gcc_64/bin:$PATH"

# Install system dependencies for Qt WebEngine
sudo apt-get update && sudo apt-get install -y \
  libglib2.0-0 libnss3 libnspr4 libdbus-1-3 libatk1.0-0 \
  libatk-bridge2.0-0 libcups2 libdrm2 libxkbcommon0 \
  libxcomposite1 libxdamage1 libxfixes3 libxrandr2 \
  libgbm1 libasound2 libatspi2.0-0 libxshmfence1 \
  libssl3 libsecret-1-0

echo "=== Qt installed at /opt/Qt/6.7.2/gcc_64 ==="
echo "Run 'source ~/.bashrc' or restart terminal (Ctrl+Shift+\`)"