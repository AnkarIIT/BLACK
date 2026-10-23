#!/bin/bash
# .devcontainer/setup-qt.sh - Install Qt6 in Codespaces

set -e

echo "=== Installing Qt 6 for BLACK Browser ==="

# Install Qt6 via official installer (non-interactive)
QT_VERSION="6.7.2"
QT_INSTALLER="qt-unified-linux-x64-online.run"
QT_URL="https://download.qt.io/official_releases/online_installers/${QT_INSTALLER}"

cd /tmp
wget -q "${QT_URL}" -O "${QT_INSTALLER}"
chmod +x "${QT_INSTALLER}"

# Install Qt6 base + webengine + svg (no GUI)
./"${QT_INSTALLER}" --verbose --script /dev/stdin <<'EOF' <<< "
Component.installationPrefix = '/opt/Qt';
Component.autoAnswer = true;
Component.installComponents = ['qt.qt6.672.gcc_64', 'qt.qt6.672.qtwebengine.gcc_64', 'qt.qt6.672.qtsvg.gcc_64'];
"
EOF

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
echo "Run 'source ~/.bashrc' or restart terminal"