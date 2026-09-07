# BLACK Browser — Changelog

## [Unreleased]

### Refactor
- **Downloads extracted into `DownloadManager`**: the download lifecycle (location prompt, progress tracking, safe auto-open, button tooltip) moved out of the 4000-line `BrowserWindow` God class into a dedicated `DownloadManager` (new `DownloadManager.h/.cpp`) that owns the download list and reports back via signals. `BrowserWindow` now only renders the downloads menu/rows from it. The `DownloadItemInfo` struct and the download auto-open allowlist moved with it.

### Added
- **Responsive platform detection**: a `black-platform` content script runs on every page (internal and external) and sets `data-platform` ("phone"/"tablet"/"desktop") on `<html>` at DocumentReady, re-evaluating on resize and orientation change. This makes the breakpoints and touch sizes in `responsive.css` actually apply on desktop, tablet and phone (previously the attribute was never set, so the responsive system was inert).
- **Touch-aware window dragging**: `BrowserWindow::event()` now handles `QTouchEvent::TouchBegin/Update/End`, letting a finger drag the frameless window from the title-bar/toolbar area and maximize it, mirroring mouse behaviour on touch devices.
- **Touch-aware widget sizing**: new `BrowserWindow::isTouchDevice()` and `touchIconSize()` scale the reload button, "new tab" button, URL bar height and tab-bar height up to comfortable touch targets on tablet/phone while keeping the compact desktop sizes.
- **Mobile build guard**: CMake now fails with a clear message (instead of a confusing configure error) when `ANDROID` or `IOS` is set, explaining that Qt WebEngine (Widgets) is desktop-only and a port to Qt Quick/QML (Android) or WKWebView (iOS) is required.
- **Viewport safe-area**: added `viewport-fit=cover` to the viewport meta tag on all local HTML pages so `env(safe-area-inset-*)` in `responsive.css` works on notched devices.

### Fixed
- **ShelfStore write-on-read bug**: `loadArray()` no longer writes to disk during read operations. Retention pruning is now performed explicitly in `add()`, `remove()`, and `importBookmarks()` before saving. This eliminates unexpected disk I/O on every URL-suggestion keystroke.
- **Homepage URL injection**: `BrowserWindow::homepageUrl()` now validates the URL scheme. Only `http`, `https`, `qrc`, `about`, and empty schemes are allowed. `javascript:` and `file://` homepages are rejected.
- **Silent permission save failure**: `BrowserWindow::savePermissions()` now shows a `QMessageBox` warning when `VaultCrypto::encrypt()` fails, so users understand why permission prompts reappear.
- **Plaintext session file**: `session.json` is now encrypted via `VaultCrypto::encrypt()`. `restoreSession()` attempts decryption first and falls back to plaintext for backward compatibility with existing installations.

### Security
- **SafeBrowsing Unicode normalization**: `SafeBrowsing::isBlocked()` now normalizes hostnames to ASCII-compatible Punycode (ACE) before matching, catching homoglyph and Unicode-based phishing domains.

### Planned (P0/P1 — requires further implementation)
- Replace custom HMAC-CTR vault cipher with AES-256-GCM or ChaCha20-Poly1305.
- Integrate vault master key with OS secure storage (Windows Credential Manager, macOS Keychain, Linux libsecret).
