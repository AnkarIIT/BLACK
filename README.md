# BLACK

[![Qt](https://img.shields.io/badge/Qt-6.8.0-41cd52)](https://www.qt.io)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://isocpp.org)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-064f8c)](https://cmake.org)
[![Chromium](https://img.shields.io/badge/Engine-Chromium%20122-white)](https://www.chromium.org)

**A local-first, Safari-inspired desktop browser with a live Chrome/Safari layout switch — built on Qt 6 and Qt WebEngine.**

BLACK re-imagines the macOS Safari visual language — traffic-light window controls, a fluid tab bar, a live tab-overview grid, and a smart sidebar — on a cross-platform native stack. It is deliberately **local-first**: no cloud accounts, no telemetry, and no sync. Everything that touches your data is encrypted on-device and never leaves the machine.

> **Note:** BLACK is built on Chromium (Blink) via Qt WebEngine with Safari-inspired UI styling. It is not WebKit and has no macOS/Apple integration.

---

## Highlights

- **Two complete chrome layouts, switchable live.** A Safari mode (floating translucent sidebar, macOS-style toolbar) and a Classic Chrome mode (docked sidebar, top tab strip, window controls on the right edge). Toggling re-arranges the native widget tree, restyles every panel, and re-themes internal pages — no restart required.
- **Encrypted-at-rest everything.** Passwords, service tokens, and permission choices are persisted in encryption envelopes via `VaultCrypto` (Windows DPAPI backed by a PBKDF2-HMAC keystream on other platforms).
- **Security-first engine configuration.** Offline Safe Browsing, tracker blocking, HTTPS-First upgrades, isolated-world scripting, and a hardened WebEngine profile out of the box.
- **Cinematic first-run onboarding.** A frameless WebGL sequence (Three.js + GSAP) that wires up real OAuth identity providers and local bookmarks import before the main window ever appears.

---

## Features

| Category | Capability |
|----------|------------|
| Browsing | Tabbed browsing, pinned tabs, manual tab groups, live tab-overview thumbnails, find-in-page, downloads, session restore, history |
| Layout | Safari ↔ Classic Chrome live switch, docked/floating sidebar, per-mode toolbar and tab strip |
| Personal data | Local favorites/bookmarks, reading list, bookmark import from installed browsers |
| Privacy | Tracker blocking, HTTPS-First upgrades, offline Safe Browsing, private/incognito windows |
| Credentials | Local encrypted password manager with autofill, encrypted service-token store |
| Services | OAuth 2.0 / PKCE connections to Google, Apple, Microsoft, GitHub, Slack, Discord, Google Drive, Calendar, Dropbox, and Notion |
| Resilience | Renderer crash recovery with automatic reload budget and a dedicated crash page; session persistence survives crashes |
| Theming | Automatic light/dark system theming, per-mode chrome palettes, `data-ui-layout`-driven internal-page theming |
| Extensions | Internal extension pages with a scoped, isolated scripting model |

**Not included (by design or not yet implemented):** cloud sync, passkeys/WebAuthn, Apple Pay / Keychain / iCloud, reader mode, PiP, translation, per-site content settings, or a uBlock-style content-blocker API.

---

## Architecture

### UI layers

The window is built once, then driven by a single layout-mode gate. Both chrome layers are always present; switching modes only moves and restyles widgets:

- **`BrowserWindow`** owns all shared behavior — tabs, navigation, the sidebar, suggestions, downloads, find, and session handling.
- **`ChromeLayer`** owns the Classic Chrome chrome: the top tab strip (`[traffic lights][tabs…][new-tab]`), window controls (macOS traffic lights on the left; minimize/maximize/close glyphs on the right edge for Windows/Linux), and the Chrome restyle of the shared toolbar.
- **`BrowserWindow::applyUiLayout()`** reads `BrowserSettings::uiLayout()`, syncs the `data-ui-layout` attribute onto every open page (including the settings sheet), toggles omnibox width, swaps the sidebar treatment, and delegates to `ChromeLayer::setChromeMode()`.
- **`ChromeTheme` / `SafariTheme`** supply the per-mode palettes; internal pages read `data-ui-layout="chrome" | "safari"` and re-theme via CSS.

Switching layouts is feature-neutral: navigation, tabs, bookmarks, history, passwords, session restore, crash recovery, and the extension bridge all behave identically in both modes.

### Rendering & scripting

- **Qt WebEngine (Chromium 122)** hosts all pages; the chrome itself is native Qt widgets.
- **Isolated-world scripting.** Injected scripts (autofill, UI helpers) run in a dedicated application world so page content can neither read nor tamper with them. Extension and page scripts stay separate from internal logic.
- **Per-page bridge scope.** The full internal bridge is exposed in `MainWorld` only for `qrc:` pages. External `http/https` pages receive a minimal, password-only bridge in a private world.

### Storage

| Store | File | Mechanism |
|-------|------|-----------|
| Passwords | `passwords.json` | Encrypted envelope, per-entry bridge lookups only |
| Service tokens | `services.json` | Encrypted via `VaultCrypto` |
| Permission choices | `permissions.json` | Encrypted, with legacy plaintext fallback |
| Bookmarks / history | `bookmarks.json`, `history.json` | Plain local JSON via `ShelfStore` |
| Settings | `settings.json` | `BrowserSettings` singleton |

---

## Security & Privacy

| Protection | Status |
|------------|--------|
| Tracker blocking (EasyList-style interceptor) | ✅ |
| HTTPS-First main-frame upgrades | ✅ |
| Offline Safe Browsing (embedded blocklist, no cloud API) | ✅ |
| Encrypted local vault (DPAPI / PBKDF2-HMAC-SHA256) | ✅ |
| Encrypted service-token store | ✅ |
| Encrypted permission storage with legacy fallback | ✅ |
| Renderer crash recovery (auto-reload budget → crash page) | ✅ |
| Isolated-world content scripts | ✅ |
| Password bridge scoped to internal pages / private autofill world | ✅ |
| Clipboard & screen-capture disabled in page settings | ✅ |
| Private / incognito windows (throwaway profile, no session persistence) | ✅ |
| Single-instance guard (`QLockFile` + URL hand-off) | ✅ |

### Onboarding hardening

- Third-party assets (**Three.js**, **GSAP**) are **vendored** into `vendor/` and served from the Qt resource bundle — no CDN, no network dependency, nothing to tamper with.
- The onboarding `QWebChannel` exposes only what the flow needs (`onboardingBridge`, `oauthManager`, `bookmarkImporter`, `appearance`, `theme`). No bookmarks store, no raw file access.

---

## First-Run Flow

```
        First launch?
        (.first_run_done)
              │
     ┌────────┴─────────┐
     ▼                  ▼
  Yes (first run)    No / done
     │                  │
     ▼                  ▼
 Onboarding dialog  Open start page
 (onboarding_experience.html)
     │
     │ (Launch / Skip)
     ▼
 Write .first_run_done
     │
     ▼
   Start Page
(startpage_enhanced.html)
```

Onboarding covers: the cinematic intro, real OAuth identity providers (Google / Apple / Microsoft), local bookmark import, personalization + privacy setup (**Maximum Privacy** is the default), and optional service connections. All profile fields and tokens stay on-device — there is **no cloud sign-in by default**.

---

## Service Configuration

OAuth buttons and service connections appear only for providers configured in `<AppDataDirectory>/oauth.json`. See [`oauth.json.example`](oauth.json.example):

```json
{
  "google":   { "clientId": "YOUR_GOOGLE_CLIENT_ID" },
  "microsoft":{ "clientId": "YOUR_MS_CLIENT_ID" },
  "apple":    { "clientId": "YOUR_APPLE_CLIENT_ID", "clientSecret": "YOUR_APPLE_ES256_JWT" },
  "github":   { "clientId": "YOUR_GITHUB_CLIENT_ID", "clientSecret": "YOUR_GITHUB_CLIENT_SECRET" },
  "slack":    { "clientId": "YOUR_SLACK_CLIENT_ID", "port": 9011 },
  "discord":  { "clientId": "YOUR_DISCORD_CLIENT_ID", "port": 9012 },
  "drive":    { "clientId": "YOUR_GOOGLE_CLIENT_ID" },
  "calendar": { "clientId": "YOUR_GOOGLE_CLIENT_ID" },
  "dropbox":  { "clientId": "YOUR_DROPBOX_CLIENT_ID", "port": 9013 },
  "notion":   { "clientId": "YOUR_NOTION_CLIENT_ID", "port": 9014 }
}
```

- Fixed-port providers redirect to `http://127.0.0.1:<port>/callback`.
- Google / Microsoft / Drive / Calendar / Apple use an ephemeral local port.
- Apple additionally requires a pre-generated ES256 `client_secret` JWT.

---

## Build Instructions

### Requirements

| Dependency | Version | Notes |
|------------|---------|-------|
| CMake | 3.16+ | |
| Qt | 6.8.0 | GCC / MSVC 2022 64-bit |
| Qt WebEngine | 6.8.0 | Chromium-based, **not** WebKit |
| OS | Linux / Windows 10+ | Fedora/Ubuntu and Windows 10/11 tested |

### Linux

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.0/gcc_64/lib/cmake
cmake --build build
./build/BLACK
```

### Windows

```powershell
# From a Developer Command Prompt (sets up the MSVC environment):
call "C:\Program Files\Microsoft Visual Studio\17\Community\VC\Auxiliary\Build\vcvars64.bat"

# Configure, build (Release), and run:
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Release
.\build\Release\BLACK.exe
```

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close current tab |
| `Ctrl+Shift+T` | Reopen last closed tab |
| `Ctrl+L` / `F6` | Focus address bar |
| `Ctrl+F` | Find in page |
| `Ctrl+R` / `F5` | Reload |
| `Ctrl+Shift+R` | Hard reload |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | Next / previous tab |
| `Ctrl+1` – `Ctrl+9` | Jump to tab 1–9 |
| `Ctrl+Shift+L` | Toggle sidebar |
| `Alt+Left` / `Alt+Right` | Back / Forward |
| `Ctrl++` / `Ctrl+-` | Zoom in / out |
| `F11` | Toggle full screen |
| `Escape` | Close find bar / overview |

---

## Project Structure

```
BLACK/
├── main.cpp                      # Entry point; first-run/onboarding gate
├── BrowserWindow.cpp/.h          # Main window, tabs, sidebar, layout-mode gate
├── ChromeLayer.cpp/.h            # Classic Chrome layer: tab strip + window controls
├── ChromeTheme.cpp/.h            # Chrome palette
├── SafariWebView.cpp/.h          # QWebEngineView subclass with context menu
├── SafariWebPage.cpp/.h          # Page lifecycle, isolated-world script injection
├── SafariTheme.cpp/.h            # Light/dark theme singleton + system scheme
├── BrowserSettings.cpp/.h        # Settings persistence + uiLayout mode
├── AppearanceManager.cpp/.h      # uiLayout bridge for internal pages
├── TrackerBlocker.cpp/.h         # Tracker blocking + HTTPS-First interceptor
├── SafeBrowsing.cpp/.h           # Offline blocklist + warning page (thread-safe)
├── PasswordStore.cpp/.h          # Local encrypted password manager
├── VaultCrypto.cpp/.h            # DPAPI / PBKDF2-HMAC encryption envelope
├── OSPaths.cpp/.h                # Platform data-directory resolution
├── BookmarkImporter.cpp/.h       # Bookmark import from installed browsers
├── ShelfStore.cpp/.h             # Local bookmarks / history stores
├── ExtensionManager.cpp/.h       # Internal extension pages
├── Account.cpp/.h                # Local profile + first-run marker
├── OAuthManager.cpp/.h           # OAuth 2.0 / PKCE + encrypted token store
├── OnboardingBridge.h/.cpp       # First-run bridge + local browser import
├── onboarding_experience.html    # Cinematic onboarding
├── vendor/                       # Vendored Three.js / GSAP (no CDN)
├── startpage_enhanced.html       # Start page
├── settings.html                 # Settings UI
├── bookmarks.html, history.html  # Bookmarks & history UI
├── extensions.html, features.html# Extensions & feature overview
├── privacyreport.html            # Privacy report
├── safebrowsing_warning.html     # Safe Browsing interstitial
├── crash.html                    # Renderer crash page
├── chrome-theme.css              # data-ui-layout="chrome" page theming
├── CMakeLists.txt                # Qt 6 CMake build
├── deploy.ps1                    # Automated deployment script
└── installer.iss                 # Inno Setup installer
```

---

## Development Notes

### First-run marker

```cpp
bool isFirstRun() {
    QFile marker(dataDir + "/.first_run_done");
    return !marker.exists();
}
```

The marker is written by `Account::completeOnboarding()` when onboarding finishes (Launch / Skip).

### Adding an isolated-world content script

```cpp
// Runs in the application world so page content cannot read or tamper with it:
webPage->runJavaScript(script, QWebEngineScript::ApplicationWorld);
```

### Renderer crash recovery

Transient renderer kills are auto-reloaded (up to `kMaxRendererCrashReloads` times); repeated crashes land on the `crash.html` page. Crashed tabs persist that state across restarts so a dead page isn't hammered on relaunch.

---

## Troubleshooting

**Build fails: "Visual Studio not found"**
```powershell
# Run from a Developer Command Prompt:
call "C:\Program Files\Microsoft Visual Studio\17\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -S . -B build ...
```

**Qt DLLs not found at runtime (Windows)**
```powershell
$env:PATH += ";C:\Qt\6.8.0\msvc2022_64\bin"
```

**Onboarding not showing**
- Delete the `.first_run_done` marker in the app data directory and relaunch.
- Verify `main.cpp` checks `isFirstRun()` before constructing the main window.

---

## Acknowledgements

- [Three.js](https://threejs.org) and [GSAP](https://greensock.com/gsap/) for the onboarding cinematics.
- Chromium / Qt WebEngine for the rendering engine.
