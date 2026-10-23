# BLACK Browser — Build Guide

## TL;DR

| Method | Time | Difficulty | Works? |
|--------|------|------------|--------|
| **GitHub Codespaces** | 5 min | Zero setup | ✅ **Yes** |
| **Local Windows** | 15 min | Install VS + Qt + CMake | ✅ Yes |
| **Local Linux** | 10 min | `apt install` | ✅ Yes |
| **GitHub Actions CI** | N/A | N/A | ❌ **Blocked** |

---

## Why GitHub Actions CI Fails

```
┌─────────────────────────────────────────────────────────────┐
│  GitHub Actions Runner (US Azure)                          │
│       │                                                     │
│       ▼                                                     │
│  download.qt.io  ◄── BLOCKED (firewall/geo-restrictions)   │
│       │                                                     │
│       ▼                                                     │
│  ALL Qt installer methods fail:                            │
│  • jurplel/install-qt-action                               │
│  • aqtinstall (pip)                                        │
│  • Official installer script                               │
│  • Manual wget from mirrors                                │
│                                                             │
│  Ubuntu apt packages: Qt 6.6 missing WebEngine             │
│  Chocolatey: qt6 package broken                            │
└─────────────────────────────────────────────────────────────┘
```

**Result:** No Windows CI possible on GitHub Actions. Linux CI fails because Ubuntu 24.04 ships Qt 6.6 without WebEngine.

**Codespaces works** because it runs on a different network with full access to Qt servers.

---

## Detailed Build Instructions

### 🟢 Method 1: GitHub Codespaces (Zero Setup, Guaranteed)

1. **Open:** https://github.com/codespaces/new?repo=AnkarIIT/BLACK
2. **Click:** "Create codespace on main"
3. **Wait:** 2-3 minutes for dev container setup
   - Base: `mcr.microsoft.com/devcontainers/cpp:ubuntu-24.04`
   - Features: CMake, Ninja
   - Post-create: `bash .devcontainer/setup-qt.sh` (installs Qt 6.7.2 + WebEngine + SVG)
4. **Build:** Terminal → `./build.sh`
5. **Download:** File Explorer → right-click `build/BLACK` → "Download"

**Codespaces gives you:** 60 free hours/month, full internet, VS Code in browser, persistent container.

---

### 🟢 Method 2: Local Windows Build

#### Prerequisites
| Tool | Version | Install |
|------|---------|---------|
| Visual Studio 2022 Community | 17.x | [Download](https://visualstudio.microsoft.com/vs/community/) → Workload: **Desktop development with C++** |
| Qt 6 | 6.7.2+ | [Qt Online Installer](https://www.qt.io/download-qt-installer) → Select **Qt 6.7.2** → **MSVC 2022 64-bit** |
| CMake | 3.16+ | [cmake.org/download](https://cmake.org/download/) → Windows x64 installer |

#### Build
```powershell
# 1. Edit build.ps1 line 4 to match your Qt install:
#    $QtPath = "C:\Qt\6.7.2\msvc2022_64"  # Default
#    $QtPath = "C:\Program Files\Qt\6.7.2\msvc2022_64"  # Alt location

# 2. Run in PowerShell (Admin recommended for PATH):
Set-ExecutionPolicy -Scope Process Bypass
.\build.ps1

# 3. Result:
#    build\Release\BLACK.exe
```

**Troubleshooting:**
- "MSBuild not found" → Run from **Developer Command Prompt** or install VS C++ workload
- "Qt not found" → Verify path in `build.ps1` line 4
- "QtWebEngine missing" → Ensure you selected "Qt WebEngine" in Qt installer

---

### 🟢 Method 3: Local Linux Build

#### Ubuntu 24.04 / Debian 12
```bash
sudo apt update
sudo apt install -y \
  qt6-base-dev qt6-webengine-dev qt6-svg-dev \
  libssl-dev libsecret-1-dev \
  cmake ninja-build g++

./build.sh
```

#### Fedora 39+
```bash
sudo dnf install -y \
  qt6-qtbase-devel qt6-qtwebengine-devel qt6-qtsvg-devel \
  openssl-devel libsecret-devel \
  cmake ninja-build gcc-c++

./build.sh
```

#### Arch Linux
```bash
sudo pacman -S \
  qt6-base qt6-webengine qt6-svg \
  openssl libsecret \
  cmake ninja gcc

./build.sh
```

---

## Build Scripts Reference

| Script | Platform | Auto-Detects Qt? | Notes |
|--------|----------|------------------|-------|
| `build.sh` | Linux / Codespaces | Yes (env, /opt/Qt, /usr/lib/qt6, qmake6) | Use in Codespaces |
| `build.ps1` | Windows | Yes (common install paths) | Edit line 4 if needed |
| `.devcontainer/setup-qt.sh` | Codespaces | N/A | Runs automatically |

---

## CI/CD Status

| Workflow | Status | Notes |
|----------|--------|-------|
| `.github/workflows/build-linux.yml` | ❌ Failed | Ubuntu Qt 6.6 missing WebEngine |
| `.github/workflows/build-windows.yml` | ❌ Failed | All Qt download mirrors blocked |

**No fix possible on GitHub Actions** — network restriction is on GitHub's infrastructure.

---

## Verifying Your Build

```bash
# Linux / Codespaces
./build/BLACK --version

# Windows
.\build\Release\BLACK.exe --version
```

Expected output:
```
BLACK Browser 1.0.0
Qt 6.7.2 (MSVC 2022 64-bit / GCC 13.2)
Chromium 122.x.x.x
```

---

## Common Issues

| Error | Solution |
|-------|----------|
| `Qt6WebEngineWidgets not found` | Install `qt6-webengine-dev` (Linux) or select WebEngine in Qt installer (Windows) |
| `qmake not found` | Add Qt `bin` to PATH or set `QT_ROOT` |
| `CMake: Visual Studio not found` | Run from Developer Command Prompt or install VS C++ workload |
| Codespaces: "Qt not found after setup" | Run `source ~/.bashrc` or reopen terminal (`Ctrl+Shift+``) |
| Windows: DLL not found at runtime | Add `C:\Qt\6.x.x\msvc2022_64\bin` to PATH |

---

## Release Build (Optimized)

```bash
# Linux / Codespaces
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Windows
cmake -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.7.2/msvc2022_64" -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

---

## Need Help?

- **Codespaces issues:** Check devcontainer logs (Terminal → "Dev Containers" output)
- **Local build issues:** Verify prerequisites, check CMake output
- **Qt WebEngine missing:** This is the #1 failure — ensure WebEngine module is installed

**Best path forward:** Use Codespaces. It's free, requires zero local setup, and just works.