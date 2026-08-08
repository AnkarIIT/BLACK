# BLACK Browser

A high-performance, Safari-inspired browser for Windows, built with **Qt 6.8.0** and **Qt WebEngine (Chromium 122)**. BLACK borrows macOS Safari's visual language — traffic-light window controls, a fluid tab bar, a live tab overview grid, and a smart sidebar — while staying fully **local-first**: no cloud accounts, no telemetry, no sync.

**Note**: BLACK uses the Chromium (Blink) engine with Safari-inspired UI styling. It is not WebKit and has no macOS/Apple integration.

## Architecture

### Core Engine
- **Qt WebEngine (Chromium 122)** with a Safari-themed chrome built in native Qt widgets.
- **Isolated-world scripting**: all injected scripts (autofill, UI helpers) run in a dedicated `ApplicationWorld`, so page content cannot tamper with or read the bridge. Extension content scripts and page scripts are kept separate from internal logic.

### Security & Privacy
- **Local encrypted vault**: saved passwords are stored in an encrypted envelope (`passwords.json`) via `VaultCrypto` — native **DPAPI** (`CryptProtectData`) on Windows, with a **PBKDF2-HMAC-SHA256**-keyed keystream fallback elsewhere. The vault is never handed out over the bridge; callers get per-entry lookups only.
- **Tracker blocking**: a `QWebEngineUrlRequestInterceptor` blocks known tracker hosts (EasyList-style list + local overrides) and records daily/weekly/30-day blocked counts.
- **HTTPS-First**: insecure `http://` main-frame loads are auto-upgraded to `https://` (except local hosts and IP literals).
- **Offline Safe Browsing**: navigation to hosts on an embedded blocklist (plus local overrides) is redirected to a warning page. Lookups run on Chromium's IO thread against a `QReadWriteLock`-guarded set — no cloud Safe Browsing API.
- **Private mode**: incognito windows use a throwaway `QWebEngineProfile` and skip session persistence.
- **Hardened WebEngine**: clipboard and screen-capture access are disabled in the page settings.

### Onboarding
- A frameless, **16-frame WebGL cinematic** first-run experience built with **Three.js & GSAP**, covering profile creation, import, personalization, and privacy level — all local, no account required.

## First-Time Login Flow

On first launch BLACK shows a login page with two honest options — there is **no cloud sign-in**:

1. **Create Local Profile** - stores a profile name on this device only.
2. **Continue as Guest** - browse without creating a profile.

```
┌─────────────────┐
│  Welcome Screen │
│   (login.html)  │
└────────┬────────┘
         │
    ┌────┴────┐
    ▼         ▼
 Local     Continue
Profile    as Guest
    │         │
    └────┬────┘
         ▼
    Start Page
(startpage_enhanced.html)
         │
         ▼
    Normal
   Browsing
```

## Features

| Feature | Status |
|---------|--------|
| Tabbed browsing, tab overview (live thumbnails) | ✅ |
| Pinned tabs & manual tab groups | ✅ |
| Find in page, downloads, session restore, history | ✅ |
| Favorites / bookmarks & reading list (local) | ✅ |
| Smart sidebar | ✅ |
| Automatic light/dark system theming | ✅ |
| Tracker blocking (EasyList-style interceptor) | ✅ |
| HTTPS-First upgrades | ✅ |
| Offline Safe Browsing with warning page | ✅ |
| Password manager (local, encrypted) + autofill | ✅ |
| Private / incognito windows | ✅ |
| macOS-style chrome & keyboard shortcuts | ✅ |
| Extension manager (internal pages) | ✅ |

### Not Included (by design / not yet implemented)
- No cloud sync, no accounts, no iCloud/Keychain, no Apple Pay
- No passkeys / WebAuthn
- No Safari Notify Me
- No reader mode, PiP, or translation
- No per-site content settings
- No content-blocker extension API (uBlock-style)

## Build Instructions (Windows)

```powershell
# 1. Set up Visual Studio environment
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

# 2. Configure (Qt 6.8.0 MSVC 2022 64-bit)
cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Codes\BLACK\Qt\6.8.0\msvc2022_64"

# 3. Build (Release)
cmake --build build --config Release

# 4. Run
cd build\Release
.\BLACK.exe
```

## Requirements

| Dependency | Version | Notes |
|------------|---------|-------|
| Windows | 10 / 11 | |
| CMake | 3.16+ | |
| Visual Studio | 2022 17+ | MSVC 14.5+ |
| Qt | 6.8.0 | MSVC 2022 64-bit |
| Qt WebEngine | 6.8.0 | **Chromium-based (not WebKit)** |

## Project Structure

```
BLACK/
├── main.cpp                      # App entry point with first-run/login flow
├── BrowserWindow.cpp/.h          # Main window, tabs, sidebar, find, downloads
├── SafariWebView.cpp/.h          # QWebEngineView subclass with context menu
├── SafariWebPage.cpp/.h          # Page lifecycle, isolated-world script injection
├── SafariTheme.cpp/.h            # Light/dark theme singleton + system scheme
├── BrowserSettings.cpp/.h        # Web engine profile and settings
├── TrackerBlocker.cpp/.h         # Request interceptor: tracker blocking + HTTPS-First
├── SafeBrowsing.cpp/.h           # Offline blocklist + warning page (thread-safe)
├── PasswordStore.cpp/.h          # Local password manager (per-entry bridge lookups)
├── VaultCrypto.cpp/.h            # DPAPI / PBKDF2-HMAC encryption envelope
├── OSPaths.cpp/.h                # Platform data-directory resolution
├── BookmarkImporter.cpp/.h       # Import bookmarks from other browsers
├── ShelfStore.cpp/.h             # Local favorites / bookmarks store
├── ExtensionManager.cpp/.h       # Internal extension pages
├── Account.cpp/.h                # Local profile + first-run marker
├── login.html                    # First-run login (local profile / guest)
├── onboarding_experience.html    # 16-frame WebGL cinematic onboarding
├── startpage_enhanced.html       # Start page
├── settings.html, bookmarks.html # Settings & bookmarks UI
├── history.html, extensions.html # History & extensions UI
├── features.html                 # Feature overview
├── privacyreport.html            # Privacy report
├── safebrowsing_warning.html     # Safe Browsing interstitial
├── CMakeLists.txt                # Qt 6 CMake build
├── deploy.ps1                    # Automated deployment script
└── installer.iss                 # Inno Setup installer
```

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
| `Ctrl+Tab` | Next tab |
| `Ctrl+Shift+Tab` | Previous tab |
| `Ctrl+1` - `Ctrl+9` | Jump to tab 1-9 |
| `Ctrl+Shift+L` | Toggle sidebar |
| `Alt+Left/Right` | Back / Forward |
| `Ctrl++` / `Ctrl+-` | Zoom in / out |
| `F11` | Toggle full screen |
| `Escape` | Close find bar / overview |

## Security

| Protection | BLACK |
|------------|-------|
| Tracker Blocking (EasyList-style interceptor) | ✅ |
| HTTPS-First upgrades | ✅ |
| Offline Safe Browsing (no cloud API) | ✅ |
| Encrypted local password vault (DPAPI / PBKDF2-HMAC) | ✅ |
| Isolated-world content scripts | ✅ |
| Password bridge scoped to internal pages / private autofill world | ✅ |
| Single-instance guard (`QLockFile` + URL hand-off) | ✅ |
| Renderer sandbox + process isolation (Chromium) | ✅ |
| Private / incognito windows | ✅ |
| Clipboard & screen-capture disabled | ✅ |
| Certificate & permission dialogs | ✅ |

## Development

### First-Run Logic
```cpp
bool isFirstRun() {
    QFile marker(dataDir + "/.first_run_done");
    return !marker.exists();
}
```
The marker is created by `Account::completeOnboarding()`, which is called from the login page (create profile / continue as guest) or when the cinematic onboarding finishes.

### Adding an Isolated-World Content Script
```cpp
// Scripts are injected into ApplicationWorld so the page cannot touch the bridge:
webPage->runJavaScript(script, QWebEngineScript::ApplicationWorld);
```

## Troubleshooting

**Build fails: "Visual Studio not found"**
```powershell
# Run from Developer Command Prompt
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -S . -B build ...
```

**Qt DLLs not found**
```powershell
# Add to PATH
$env:PATH += ";C:\Codes\BLACK\Qt\6.8.0\msvc2022_64\bin"
```

**Login page not showing**
- Check that `login.html` is in resources
- Verify `main.cpp` checks `isFirstRun()` correctly
