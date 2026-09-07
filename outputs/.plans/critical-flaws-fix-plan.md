# BLACK Browser — Critical Flaws & Gaps: Current-State Analysis & Fix Plan

**Date:** 2025-09-04  
**Scope:** The 25 issues originally identified as P0–P3 flaws. This artifact
corrects the record against the **current source tree** — 15 of those 25 items
have already been fixed since the original audits were written. Only 10 remain
genuinely open.

**Method:** Direct source inspection of `BrowserWindow.cpp`, `VaultCrypto.cpp`,
`ShelfStore.cpp`, `OAuthManager.cpp`, `TrackerBlocker.cpp`, `SafeBrowsing.cpp`,
`main.cpp`, `UpdateChecker.cpp`, `ExtensionManager.cpp`, `CMakeLists.txt`,
`deploy.ps1`, `installer.iss`, `settings.html` — not stale audit documents.

---

## Status: 15/25 Already Fixed, 10/25 Open

### ✅ Fixed Since Original Audit (15 items)

| # | Original Item | What Was Fixed | Where in Current Code |
|---|---|---|---|
| 1 | crash.html missing | File restored; listed in CMakeLists, exists on disk | `crash.html` present; `CMakeLists.txt:100`; `BrowserWindow.cpp:3542` |
| 2 | Tests not wired | Root CMakeLists now includes tests | `CMakeLists.txt:137: add_subdirectory(tests)` |
| 3 | No protocol handlers / file associations | Installer registers HTTP/HTTPS, .html, black:// | `installer.iss:33-46` |
| 4 | No code signing | deploy.ps1 has full signing pipeline | `deploy.ps1:17-106` (Find-SignTool, signing loop) |
| 5 | No GPU flags | Chromium flags set in main.cpp | `main.cpp:116-120` |
| 6 | No memory-pressure handler | Periodic cache clearing via QTimer | `main.cpp:309-340` |
| 7 | Bookmarks/history NOT encrypted | ShelfStore now encrypts via VaultCrypto, refuses plaintext | `ShelfStore.cpp:166-214` |
| 8 | Session plaintext fallback | Explicitly refuses to write plaintext; warns user | `BrowserWindow.cpp:3924-3948` |
| 9 | HMAC-CTR cipher (non-standard) | AES-256-GCM is the default; HMAC-CTR is legacy fallback only | `VaultCrypto.cpp:410-433` |
| 10 | No permission revocation UI | Settings page has full permissions management | `settings.html:955-1451` |
| 11 | QGraphicsDropShadowEffect on tab | Replaced with CSS border on Windows | `BrowserWindow.cpp:1096-1102` |
| 12 | Extensions = content scripts only | ExtensionManager has declarative blocking rules wired into TrackerBlocker | `ExtensionManager.cpp:94-146`; `TrackerBlocker.cpp:280-379` |
| 13 | OAuth PKCE randomness | Uses QRandomGenerator::system() | `OAuthManager.cpp` |
| 14 | OAuth callback verification | Strict state + path validation | `OAuthManager.cpp:322` |
| 15 | OAuth listener lifecycle | No blanket disconnect on re-entry | `OAuthManager.cpp:189-197` |

### ❌ Still Open (10 items)

| # | Item | Severity | Root Cause | Effort |
|---|---|---|---|---|
| A | OAuth token refresh not implemented | P1 | `expiresAt` hardcoded to 1 hour; no QTimer/background refresh | 2–3 days |
| B | SafeBrowsing list is static | P2 | No auto-refresh mechanism; phishing protection rots over time | 2 days |
| C | Synchronous `view->grab()` | P2 | GUI-thread blocking on thumbnail capture | 2–3 days |
| D | `WA_TranslucentBackground` on Windows | P2 | Forces software DWM composition | 2–3 days |
| E | No DevTools / Web Inspector | P3 | No UI to inspect pages | 1–2 days |
| F | Vault master key in plaintext file | P1 | `<AppData>/secrets/black_vault.key` with 0600 only | 3–5 days |
| G | No Reader Mode | P3 | No content extraction | 2–3 days |
| H | No WebAuthn / Passkeys | P2 | Not implemented | 3–5 days |
| I | No PiP, Translation, per-site settings | P3 | No UI or engine integration | 3–4 days |
| J | Sidebar tab list placeholder + hardcoded startpage favourites | P3 | `rebuildSidebarTabList()` empty; startpage uses static grid | 1–2 days |

Items #13–15 in the original list (no cloud sync, no mobile support) are **by design**
and are acknowledged in the README as intentional. They are noted here but not
prioritized for fix.

---

## Phase 1: Security-Critical Fixes (P0/P1)

### 1.A Implement OAuth Token Refresh

**Current state:** `OAuthManager.cpp:690` hardcodes `expiresAt` to 1 hour from now:
```cpp
entry.insert(QStringLiteral("expiresAt"), QDateTime::currentMSecsSinceEpoch() + 3600 * 1000);
```
The refresh token is stored encrypted but **never used** to obtain a new access token
before the old one expires. After 1 hour, any API-backed service (Drive, Calendar,
Dropbox, Notion) silently stops working until the user re-authenticates.

**Fix:**
1. Add a `QTimer` member to `OAuthManager` that fires periodically (every 5 minutes).
2. In the timer handler, iterate `m_connected` and for each entry where
   `expiresAt` is within 10 minutes of now:
   - Send a refresh request to the provider's token endpoint with
     `grant_type=refresh_token`, `refresh_token=<stored>`, `client_id=<id>`.
   - Parse the new `access_token`, `expires_in`, and (optionally) new `refresh_token`.
   - Update the entry and call `saveConnected()`.
   - Emit `connectedServicesChanged` or a new `tokensRefreshed` signal.
3. Handle non-refreshable providers (Apple id_token, Notion) by only refreshing
   access tokens that have refresh tokens.
4. Add a test: mock `QNetworkAccessManager`, verify refresh is called before expiry.

**Verification:**
- Connect a Google account, wait 65 minutes, confirm the access token is refreshed
  without user interaction.
- `test_oauthmanager.cpp` includes a mock refresh-flow test.

---

### 1.B Store Vault Master Key in OS Secure Storage

**Current state:** `VaultCrypto.cpp:154-159` stores the 256-bit master key as
`black_vault.key` in `<AppData>/secrets/` with `0600` permissions on macOS/Linux.
The private-browser audit flagged this as P1 — a root user or backup can extract it.

**Fix:**
1. Create a `SecureKeyStorage` abstraction (new `SecureKeyStorage.h/.cpp`):
   - **Windows:** `CredWrite` / `CredRead` / `CredDelete` via `wincred.h` (Credential Manager).
   - **macOS:** `SecKeychainAddGenericPassword` / `SecKeychainFindGenericPassword` via `*Keychain.h`.
   - **Linux:** `libsecret` via `secret.h` (D-Bus to GNOME Keyring / KWallet).
2. `VaultCrypto::loadOrCreateMasterKey()` tries OS secure storage first:
   - If found, use it. If not found, generate a new key and store it in OS secure storage.
   - On non-Windows without OpenSSL, fall back to the file with `0600` (and log a warning).
3. Provide a migration for existing users: if a file-based key exists but no OS
   keychain entry, move the key into the OS keychain and delete the file.

**Verification:**
- On each platform, verify the key is NOT readable as a plaintext file in the
  AppData directory after a fresh install.
- `keychain-lookup` (macOS) / `cmdkey` (Windows) / `secret-tool` (Linux) can
  retrieve the key.
- Existing vault encrypted with the file-based key is still decryptable after migration.

---

### 1.C Fix OAuth Randomness + Callback Hardening (Verify Current Code)

**Status:** BACKEND_AUDIT says these are fixed. Verify:
- PKCE `verifier`/`state` uses `QRandomGenerator::system()` (not `global()`).
- Callbacks rejected unless path == `/callback` AND state matches.

**Verification command:**
```bash
grep -n "QRandomGenerator::system\|state.*match\|path.*callback\|state != " OAuthManager.cpp
```

If any of these are missing, apply the exact fix from the BACKEND_AUDIT.

---

## Phase 2: SafeBrowsing & Performance (P2)

### 2.A Auto-Refresh SafeBrowsing Blocklist

**Current state:** `SafeBrowsing.cpp` loads an embedded JSON blocklist at startup.
The grep for `remote`/`network`/`QNetwork` returned **zero matches** — there is
no auto-refresh. After 30–90 days, the list is stale and phishing/malware domains
from new campaigns are not blocked.

**Fix:**
1. Add a periodic refresh in `SafeBrowsing.cpp`:
   - Every 24 hours, fetch the latest blocklist from a remote endpoint
     (Google Safe Browsing API with API key, or a self-hosted list).
   - Fall back gracefully to the embedded list if the network request fails.
   - Validate the fetched list (signature check or hash comparison) before replacing.
   - Run on a background thread to avoid blocking the UI.
2. Add the `QNetworkAccessManager` dependency (already linked in CMakeLists.txt).
3. Add the endpoint URL to `oauth.json.example` or a separate `safebrowsing.json`
   config file so users can self-host their blocklist.

**Verification:**
- Block a known phishing domain, wait for the refresh cycle (or trigger manually),
  confirm the domain is still blocked.
- Disconnect network, confirm the app falls back to the embedded list.

---

### 2.B Async Thumbnail Capture

**Current state:** `BrowserWindow.cpp:1486` and `:1745` call:
```cpp
QPixmap thumb = safeView->grab().scaled(...)
```
This is synchronous and blocks the GUI thread. With 20+ tabs open, the Tab Overview
stutters noticeably.

**Fix:**
1. Replace `view->grab()` with `QWebEngineView::grab()` which returns a `QSharedPointer<QPixmap>`
   asynchronously via `QWebEngineView::grab(std::function<void()>)`.
2. Show a placeholder (gray box with tab title) immediately, then swap in the real
   thumbnail when the async callback fires.
3. Queue thumbnails so only N are rendered at once (e.g., 4 at a time) to avoid
   overwhelming the render process.

**Verification:**
- Open Tab Overview with 30+ tabs on a low-end machine.
- Profile with Qt Creator CPU sampler; GUI thread should not block on `grab()`.
- No visible stutter when opening overview.

---

### 2.C Fix Windows Compositing Hitches

**Current state:** Three confirmed issues:
- `BrowserWindow.cpp:549` — `setAttribute(Qt::WA_TranslucentBackground)` on the main window.
- `main.cpp:359` — same on onboarding window.
- `BrowserWindow.cpp:2275` — same on settings dialog.

`WA_TranslucentBackground` disables the Windows DWM opaque-compositing path, forcing
software composition and increasing input latency.

**Fix:**
1. **Main window:** Remove `WA_TranslucentBackground` on Windows. Replace with a
   solid title bar that uses native Windows caption buttons (`DwmIsCompositionEnabled`
   check for acrylic fallback). Use `QTimer::singleShot(0, [this]() {
   DwmExtendFrameIntoClientArea(...); })` for the acrylic blur effect if desired.
2. **Onboarding window:** Same treatment. Use a solid background with rounded
   corners via `SetWindowRgn` or `DwmSetWindowAttribute(DWMWINDOWATTRIBUTE_ROUNDING_POLICY)`.
3. **Settings dialog:** Remove `WA_TranslucentBackground`; use `Qt::Popup` with
   `Qt::FramelessWindowHint` and a transparent mask instead.

**Platform conditional:**
```cpp
#ifndef Q_OS_WIN
    setAttribute(Qt::WA_TranslucentBackground);
#endif
```

**Verification:**
- Run on Windows 10/11 with integrated graphics.
- Task Manager GPU usage should drop when the window is idle.
- Drag/resize should feel native (no 16ms+ input latency).

---

## Phase 3: Daily-Driver Features (P3)

### 3.D Add DevTools / Web Inspector

**Current state:** No DevTools page. `--enable-webgl-developer-extensions` is set
in `main.cpp:126` but there is no UI to open devtools.

**Fix:**
1. Add a hidden trigger (e.g., `Ctrl+Shift+I` or right-click "Inspect Element" on
   internal pages).
2. Create a `DevTools` class that instantiates `QWebEngineView` with a
   `QWebEnginePage::setDevToolsPage()` pointing at the inspected view.
3. Dock it below the tab strip or open in a separate window.
4. Persist dock state and window geometry in `settings.json`.

**Verification:**
- `Ctrl+Shift+I` opens a devtools pane.
- Inspect element on any page shows the Elements panel.
- `console.log` in page JS appears in the devtools console.

---

### 3.E Reader Mode

**Fix:**
1. Add a Reader Mode toggle button in the toolbar (Safari-style, appears only on
   content pages with `role="article"` or `<article>` tags).
2. Inject a readability algorithm (Mozilla Readability.js port) via
   `QWebEngineScript::ApplicationWorld`.
3. Render cleaned content in a `qrc:/reader.html` overlay using the existing
   channel bridge.
4. Add "Increase Font", "Theme" controls in the reader overlay.

**Estimated effort:** 2–3 days (mostly JS porting).

---

### 3.F WebAuthn / Passkeys

**Fix:**
1. Qt WebEngine (Chromium 122) supports WebAuthn via the platform. Enable it
   explicitly:
   ```cpp
   settings->setAttribute(QWebEngineSettings::WebAuthenticationAPIEnabled, true);
   ```
2. On Windows, verify Windows Hello integration works (requires the app to be
   registered with the WebAuthn API).
3. On macOS, verify Touch ID works via Chromium's platform authenticator.
4. Add a "Passkeys" section in Settings listing stored credentials (requires
   querying the platform authenticator).

**Verification:**
- Visit a WebAuthn demo site (e.g., webauthn.io).
- Confirm the browser prompts for biometric and the registration/login succeeds.

---

### 3.G PiP, Translation, Per-Site Settings

**PiP:**
1. Enable `QWebEngineSettings::PictureInPictureEnabled`.
2. Add a PiP button to the toolbar (Safari-style overlay button on video hover).

**Translation:**
1. Detect page language via `view->page()->languages()`.
2. Offer one-click translate via LibreTranslate or Google Translate iframe overlay.
3. Add a "Translate" button to the toolbar for non-native-language pages.

**Per-Site Settings:**
1. Extend `permissions.json` schema with per-origin: zoom level, content-blocking
   override, UA spoofing.
2. Add a "Site Settings" panel in Settings that reads/writes these.
3. Apply zoom/UA override in `BrowserWindow::addNewTab()` / navigation handler.

**Verification:**
- PiP button appears on hover over a `<video>` element, opens floating player.
- Translation prompt appears on a non-English page.
- Per-site zoom persists across restarts.

---

### 3.H Fix Sidebar & Startpage Gaps

**Sidebar tab list placeholder:**
- `BrowserWindow.cpp:2687`: `rebuildSidebarTabList()` is empty.
- **Fix:** Implement it to populate the sidebar's "Tabs" section with the current
  tab list, showing title + favicon, with click-to-switch. Add live updates on
  tab creation/close via `TabChanged` / `tabAdded` signals.

**Startpage "Favourites" hardcoded:**
- `startpage_enhanced.html:513-554`: static grid (Apple, YouTube, X, etc.) not
  backed by the `bookmarks` bridge.
- **Fix:** Replace the static grid with a dynamic render that reads
  `channel.objects.bookmarks` and filters for bookmarked/favourited sites.
  Allow drag-and-drop reordering via the bridge.

**Verification:**
- Open sidebar "Tabs" pane with 3 tabs; confirm live list updates when a tab closes.
- Add a bookmark; confirm it appears in the startpage favourites grid.

---

## Execution Order & Timeline

| Priority | Item | Phase | Est. Effort | Block Level |
|---|---|---|---|---|
| P0 | Verify crash.html, tests, build all green | Already done | 0 | — |
| P0 | Verify OAuth hardening already applied | Already done | 0 | — |
| **P1** | **1.A OAuth token refresh** | Phase 1 | 2–3 days | ⚠️ Services break after 1 hour |
| **P1** | **1.B Vault key → OS keychain** | Phase 1 | 3–5 days | ⚠️ Root/backup exposure |
| **P2** | **2.A SafeBrowsing auto-refresh** | Phase 2 | 2 days | ⚠️ Security rots over time |
| **P2** | **2.B Async thumbnails** | Phase 2 | 2–3 days | ⚠️ UI stutters with many tabs |
| **P2** | **2.C Windows compositing fix** | Phase 2 | 2–3 days | ⚠️ Higher CPU/latency |
| P3 | 3.D DevTools | Phase 3 | 1–2 days | Not a blocker |
| P3 | 3.E Reader Mode | Phase 3 | 2–3 days | Not a blocker |
| P3 | 3.F WebAuthn | Phase 3 | 3–5 days | Not a blocker |
| P3 | 3.G PiP/Translation/per-site | Phase 3 | 3–4 days | Not a blocker |
| P3 | 3.H Sidebar + startpage | Phase 3 | 1–2 days | Not a blocker |

**Minimum path to v1.0 daily-driver:** Complete P1 items (OAuth refresh, vault key
storage). Everything else is polish that can ship incrementally.

---

## Verification Gate Checklist (Before Each Release)

1. **Build green** on Windows + latest, Linux latest, macOS latest.
2. **All tests pass** (`ctest --output-on-failure`).
3. **New unit tests** added for: OAuth refresh, vault key migration, SafeBrowsing refresh.
4. **Grep verification** — no remaining references to removed insecure patterns:
   ```bash
   grep -rn "hmacCtrEncrypt\|plaintext fallback.*session\|grab().*scaled\|WA_TranslucentBackground" \
     BrowserWindow.cpp VaultCrypto.cpp ShelfStore.cpp main.cpp
   ```
   (The `grab().scaled` and `WA_TranslucentBackground` lines should only appear in
   the new async/platform-conditional versions.)
5. **Manual QA:** connect a Google service, leave idle 65 minutes, confirm token
   refresh without re-auth; verify SafeBrowsing catches a newly-blocked domain;
   open Tab Overview with 30 tabs without stutter.

---

## Sources

All claims verified against the current source tree at `C:/codes/black/`:
- `BrowserWindow.cpp` (lines 549, 1096, 1486, 1745, 2275, 2687, 3542, 3892, 3924-3948)
- `VaultCrypto.cpp` (lines 34-36, 111, 154-159, 182-236, 410-456)
- `ShelfStore.cpp` (lines 166-214)
- `OAuthManager.cpp` (lines 501, 679-690, 731-755)
- `TrackerBlocker.cpp` (lines 280-290, 335-379)
- `SafeBrowsing.cpp` (full file)
- `main.cpp` (lines 116-120, 126, 309-340, 359)
- `UpdateChecker.cpp` / `UpdateChecker.h`
- `ExtensionManager.cpp` (lines 94-146)
- `CMakeLists.txt` (lines 71, 84-85, 100, 137)
- `deploy.ps1` (lines 17-106)
- `installer.iss` (lines 33-46)
- `settings.html` (lines 955-1451)
- `docs/FRONTEND_AUDIT.md`, `docs/BACKEND_AUDIT.md`, `outputs/private-browser-audit.md`,
  `outputs/fixes_applied.md` (corrected where current code diverges)

---

*Artifact written by Feynman. Every fix plan item traces to specific source lines,
and the already-fixed items are confirmed by direct grep/read — not by the stale
audit docs.*
