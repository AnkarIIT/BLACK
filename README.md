# BLACK Browser

A high-performance, Safari-inspired browser for Windows, built with **Qt 6** and **Qt WebEngine**. BLACK delivers macOS Safari aesthetics — traffic-light window controls, a fluid tab bar, a live tab overview grid, a smart sidebar, and a custom start page — with automatic **light/dark system theming**, full security features, and production-ready distribution packaging.

## Features

- **macOS-style window chrome** — custom traffic-light controls (close / minimize / maximize), draggable title bar, double-click to maximize.
- **Tabbed browsing** — open, close, switch tabs; `Ctrl+Shift+T` reopens the last closed tab (up to 20).
- **Tab Overview** — live screenshot thumbnails grid of all open tabs with one-click close.
- **Smart Sidebar** — collapsible panel with real-time search filtering across tabs, bookmarks, and history.
- **Find in Page** — `Ctrl+F` inline search drawer with match count and forward/backward navigation.
- **Unified address bar** — focus ring, live URL updates, HTTPS security shield badge (green/red), loading progress bar.
- **BLACK start page** — dynamic time-of-day greeting, search box, Favorites grid, Privacy Report, Reading List, and iCloud Tabs sections.
- **Download Manager** — intercepts downloads, tracks progress, and shows a downloads popup menu.
- **Session Restore** — saves open tabs on exit (`session.json`) and restores on launch.
- **History & Bookmarks** — auto-records visited pages to `history.json`, saves bookmarks to `bookmarks.json`.
- **Security** — multi-process renderer isolation, HTTPS connection shield, macOS Safari 17.5 user-agent.
- **Automatic theming** — follows Windows light/dark mode with `#0a84ff` accent, `#2c2c2e` dark surfaces. Start page uses `prefers-color-scheme`.
- **Keyboard shortcuts** — full tab management, navigation, zoom, full screen, and find.

## Requirements

| Dependency   | Version                     |
|--------------|-----------------------------|
| Windows      | 10 / 11                     |
| CMake        | 3.16+                       |
| Compiler     | MSVC (Visual Studio 2022)   |
| Qt           | 6.x (WebEngineWidgets, Svg) |

> Tested with Qt **6.8.0** (MSVC 2022 64-bit).

## Build

```powershell
# 1. Configure
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Codes\browser\Qt\6.8.0\msvc2022_64"

# 2. Build (Release)
cmake --build build --config Release
```

## Deploy

```powershell
# Bundle Qt DLLs + WebEngine assets
.\deploy.ps1

# Build installer (requires Inno Setup)
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer.iss
```

This generates `BLACK_Setup_v1.0.exe` — a standalone Windows installer.

## Run

```powershell
$env:PATH = "C:\Codes\browser\Qt\6.8.0\msvc2022_64\bin;$env:PATH"
.\build\Release\BLACK.exe
```

## Keyboard Shortcuts

| Shortcut              | Action                         |
|-----------------------|--------------------------------|
| `Ctrl+T`              | New tab                        |
| `Ctrl+W`              | Close current tab              |
| `Ctrl+Shift+T`        | Reopen last closed tab         |
| `Ctrl+L` / `F6`       | Focus address bar              |
| `Ctrl+F`              | Find in page                   |
| `Ctrl+R` / `F5`       | Reload                         |
| `Ctrl+Shift+R`        | Hard reload                    |
| `Ctrl+Tab`            | Next tab                       |
| `Ctrl+Shift+Tab`      | Previous tab                   |
| `Ctrl+1` … `Ctrl+9`   | Jump to tab 1–9 (9 = last tab) |
| `Ctrl+Shift+L`        | Toggle sidebar                 |
| `Alt+Left/Right`      | Back / Forward                 |
| `Ctrl++` / `Ctrl+-`   | Zoom in / out                  |
| `Ctrl+0`              | Reset zoom                     |
| `F11`                 | Toggle full screen             |
| `Escape`              | Close find bar / overview      |

## Project Structure

```
browser/
├── main.cpp              # App entry point, engine config, window sizing
├── BrowserWindow.cpp/.h  # Main window: chrome, tabs, sidebar, overview, find, downloads
├── SafariTheme.cpp/.h    # Light/dark theme singleton + system scheme detection
├── SafariWebView.cpp/.h  # QWebEngineView subclass with context menu
├── startpage.html        # BLACK start page (Favorites, Privacy Report, …)
├── resources.qrc         # Bundles startpage.html into the executable
├── resources.rc          # Windows resource file for app.ico embedding
├── app.ico               # Application icon (multi-size Windows ICO)
├── installer.iss         # Inno Setup script → BLACK_Setup_v1.0.exe
├── deploy.ps1            # Automated build & windeployqt packaging script
└── CMakeLists.txt        # Qt 6 CMake build (AUTOMOC / AUTORCC)
```
