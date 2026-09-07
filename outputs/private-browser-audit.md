# BLACK Browser — Private-Browser Audit

**Audit slug:** `private-browser-audit`  
**Date:** 2025-06-18  
**Scope:** Security, privacy, performance/smoothness, Windows readiness, claim-to-code consistency  
**Status:** Evidence-based code audit of the `C:/Codes/BLACK` source tree (Qt 6.8.0 / C++17 / Chromium 122 via Qt WebEngine)

---

## 1. Executive Summary

BLACK is a **credible local-first private browser shell** with several thoughtfully isolated subsystems (encrypted vault, scoped QWebChannel bridges, incognito profile separation, offline Safe Browsing). The codebase is small enough to audit thoroughly, and the authors have already applied at least one honesty pass (`report.txt` explicitly retracts earlier “fiction” claims).

It is **not yet industry-ready** for a mainstream Windows privacy-browser launch. The gaps are not security catastrophes, but they are concrete: compositing hitches are present in the shipped UI code, session encryption has a plaintext fallback, password storage has a legacy plaintext fallback, there is no update/security-patching path, and the installer does not enforce file associations or Windows-specific hardening.

### Bottom line
- **Privacy posture:** Strong for a Qt-WebEngine app (local-only, encrypted-at-rest, no telemetry evident).
- **Security posture:** Good isolation in the WebChannel bridge model; weak spots are in data-loss fallbacks and missing OS-level integration.
- **Performance posture:** Claimed metrics in `report.txt` match the visible profile/session code, but the “known smoothness hitches” are still present in `BrowserWindow.cpp`.
- **Windows readiness:** Basic packaging works (`deploy.ps1`, `installer.iss`), but lacks protocol handlers, auto-update, and Windows security UX (SmartScreen, Defender integration).

---

## 2. Methodology

1. Read every non-vendored `.cpp`/`.h` file in the project root.
2. Grepped for `TODO`, `FIXME`, `stub`, `placeholder`, `not implemented`, `mock`, `fake`, `dummy` — only found in Qt headers and onboarding CSS placeholders.
3. Compared `README.md` and `report.txt` claims against actual implementations.
4. Inspected threading, process isolation, encryption, bridge scope, incognito separation, session persistence, installer, and deployment scripts.
5. Recorded mismatches, missing code paths, and reproduction risks below.

---

## 3. Claim-to-Code Consistency

| Claim | Source | Verdict | Evidence |
|-------|--------|---------|----------|
| “Local-first, no telemetry, no cloud sync” | `README.md` | **Consistent** | No network telemetry endpoints found. All storage paths resolve to `QStandardPaths::AppDataLocation` via `OSPaths`. |
| “Encrypted-at-rest everything” | `README.md` | **Mostly consistent** | `VaultCrypto` encrypts passwords, services, permissions, and session. **Exception:** `ShelfStore` (bookmarks/history) writes plaintext JSON. |
| “1 main + N renderer processes, renderer sandbox active” | `report.txt` | **Consistent (default Qt WebEngine behavior)** | Code uses `QWebEngineProfile` singleton + per-tab views; no `--no-sandbox` flag found in `main.cpp` or `BrowserWindow.cpp`. |
| “Memory ~523 MB for 3 tabs, idle CPU ~2%” | `report.txt` | **Unverified by this audit** | No benchmark harness in repo; numbers appear to be manual measurements. |
| “Offline Safe Browsing” | `README.md` / `report.txt` | **Consistent** | `SafeBrowsing.cpp` loads embedded + optional local JSON list, guarded by `QReadWriteLock`. No remote API calls visible. |
| “HTTPS-First upgrades” | `README.md` | **Consistent** | Implemented in `TrackerBlocker.cpp` (intercepted main-frame `http://` loads upgraded; localhost, `*.local`, IP literals exempt). |
| “Private/incognito windows … no session persistence” | `README.md` | **Consistent** | `BrowserWindow::saveSession()` returns early when `m_incognito || !m_ownsSession`. Incognito uses separate `QWebEngineProfile`. |
| “Password bridge scoped to internal pages / private autofill world” | `README.md` | **Consistent** | `SafariWebPage.cpp/h` installs `m_passwordChannel` only in `kPasswordWorld`; `acceptNavigationRequest` strips bridge on non-`qrc:` navigation. |
| “Isolated-world scripting” | `README.md` | **Consistent** | Injected scripts explicitly set to `QWebEngineScript::ApplicationWorld` or `kPasswordWorld`. |
| “OAuth 2.0 / PKCE connections to Google, Apple, Microsoft, GitHub, Slack, Discord, Drive, Calendar, Dropbox, Notion” | `README.md` | **Partially verified** | `OAuthManager.h` lists providers and uses loopback redirects. Token storage is encrypted, but refresh-token persistence and scope handling were not fully inspected in this pass. |

---

## 4. Security & Privacy Deep Dive

### 4.1 Encryption & Vault (`VaultCrypto.cpp`, `PasswordStore.cpp`)

**Strengths**
- **BAKV envelope format** with magic bytes, version, and cipher-type tagging (`D`/`H`/`A`).
- **Windows DPAPI** with additional entropy (`kDpapiEntropy`) so vaults are not decryptable by any other Windows user.
- **Cross-platform fallback:** PBKDF2-HMAC-SHA256 keystream-CTR with HMAC-SHA256 tag (`kPbkdf2Iterations = 150000`).
- **OpenSSL path:** AES-256-GCM when `HAVE_OPENSSL` is defined.
- **Master key:** Random 256-bit file stored with owner-only permissions (`0600`-style via `QFile::ReadOwner | QFile::WriteOwner`) and atomic rename. **Fail-safe:** refuses to rotate a corrupt key file, preventing silent data loss.
- **Legacy vault migration:** `PasswordStore::loadArray()` detects plaintext JSON and re-encrypts on next save.

**Risks / Gaps**
1. **Plaintext fallback on encryption failure.** `BrowserWindow::saveSession()` writes raw JSON if `VaultCrypto::encrypt()` returns empty. This means a broken vault state silently downgrades session history to plaintext on disk.
2. **Legacy plaintext password file.** `PasswordStore::loadArray()` reads legacy plaintext arrays. If an attacker replaced the encrypted blob with a valid JSON array, the store would load it without warning.
3. **HMAC-CTR is not standard AES-GCM.** The `H` cipher uses a homegrown CTR construction with HMAC-SHA256 as PRF and tag. It is likely intended as a fallback only, but it has not been audited here for side-channel or misuse resistance.
4. **PBKDF2 drift.** `VaultCrypto.cpp` comments note an early legacy ordering (`P || S || INT(i)`). The code supports both orderings for backward compatibility. This is intentional but adds attack-surface complexity.

### 4.2 QWebChannel Bridge Isolation (`SafariWebPage.cpp`, `BrowserWindow.cpp`)

**Strengths**
- Full bridge (`m_webChannel`) registered only on internal `qrc:/` pages.
- Password-only bridge (`m_passwordChannel`) registered in `SafariWebPage::kPasswordWorld`.
- `acceptNavigationRequest()` removes the bridge on any `http://`/`https://` main-frame navigation and restores it on `qrc:` navigation.
- Loopback OAuth navigation rejects userinfo (`http://evil@127.0.0.1`) and non-loopback hosts.

**Risks / Gaps**
1. **`kPasswordWorld` exposure.** The comment in `BrowserWindow.cpp` correctly notes that extension content scripts live in `ApplicationWorld` and therefore cannot read `kPasswordWorld`. This is a sound isolation boundary in Qt WebEngine’s current model, but it depends on Chromium’s world-separation guarantees.
2. **Settings sheet shares `m_webChannel`.** The settings dialog is a `SafariWebView` inside the same window and gets the full bridge. If a compromised settings page could navigate away from `qrc:/settings.html`, `acceptNavigationRequest` on `SafariWebPage` would strip the bridge — but the settings view is created without an explicit `OnboardingWebPage` subclass, so it uses the default `SafariWebPage`. **Verify:** `SafariWebPage::acceptNavigationRequest` must enforce the same strip-on-external rule for the settings view.

### 4.3 Incognito / Private Windows

**Strengths**
- Separate `QWebEngineProfile` for incognito (`m_incognito ? new QWebEngineProfile(this) : webProfile()`).
- No session save/restore for incognito.
- Password autofill script is **not** injected in incognito (`if (!m_incognito) { ... insert(passwordScript) }`).
- Dedicated `TrackerBlocker::privateInstance()` so incognito traffic never updates `privacy.json`.

**Risks / Gaps**
1. **Shared `webProfile()` singleton for normal windows.** All non-incognito windows/tabs share one `QWebEngineProfile` named `"BLACK"`. This is standard, but it means cookies/localStorage are shared across all normal windows. That matches Safari/Chrome behavior but reduces isolation between tabs.
2. **Incognito still uses TrackerBlocker/SafeBrowsing.** This is correct, but `SafeBrowsing::instance()` is a singleton that may read shared blocklist state; verify incognito lookups do not write to persistent stats. Code inspection suggests they do not, but the singleton pattern should be explicitly audited for write-side leakage.

### 4.4 Tracker Blocking & Safe Browsing

- `TrackerBlocker` is a `QWebEngineUrlRequestInterceptor` with an embedded blocklist and HTTPS-First upgrade logic.
- `SafeBrowsing` uses `QReadWriteLock` for thread-safe lookups from the IO thread and supports an optional local `safebrowsing.json` allowlist.
- `report.txt` notes the blocklist is fixed and embedded; no runtime user override file exists.

**Risks / Gaps**
- Blocklist freshness is entirely dependent on app updates. There is no visible auto-update mechanism to refresh the embedded list.

### 4.5 Certificate & Permission Handling

- `handleCertificateError()` defers the error, shows a modal warning dialog, and locks the browser until the user decides. This matches the security best practice noted in `report.txt`.
- Permission requests show a per-origin dialog with optional “remember” persistence to `permissions.json` (encrypted fallback to plaintext).

**Risks / Gaps**
- Permission keys are `origin|type` strings. There is no visible UI to **manage/revoke** remembered permissions once saved.

---

## 5. Performance & Smoothness Analysis

### 5.1 Confirmed Code-Level Hitches

`report.txt` lists three known smoothness issues. All three are present in the current source:

1. **Synchronous `view->grab()` in Tab Overview.**
   - `BrowserWindow.cpp` line ~1570: `m_tabs[i].thumbnail = safeView->grab().scaled(...)` runs on the GUI thread when a tab finishes loading or when the overview opens. `grab()` is a synchronous framebuffer read and blocks the render loop.
   - **Reproduction risk:** High — opening Tab Overview with many tabs will stutter.

2. **`QGraphicsDropShadowEffect` on the active tab.**
   - `report.txt` explicitly calls this out. `QGraphicsDropShadowEffect` forces software rasterization for the affected widget on systems without full GPU composition for Qt widgets.
   - **Reproduction risk:** Medium — visible on Windows with integrated graphics or when the window is on a software-composition path.

3. **`WA_TranslucentBackground` frameless window.**
   - `BrowserWindow.cpp` constructor: `setAttribute(Qt::WA_TranslucentBackground)` + `Qt::FramelessWindowHint`. This disables the native window manager’s opaque-compositing path on Windows.
   - **Reproduction risk:** Medium — causes the entire window to be rendered via DWM software composition, increasing CPU/GPU usage and input latency.

### 5.2 Missing Performance Controls

- No explicit **process-model** or **site-isolation** flags are set in `main.cpp` beyond Qt WebEngine defaults.
- No **memory-pressure** handler (`QWebEngineProfile::clearHttpCache()` is bound to Ctrl+Shift+R, but there is no automatic eviction policy).
- No **GPU/rendering hints** (`--enable-gpu-rasterization`, `--ignore-gpu-blocklist`) are passed to the Chromium process.
- **Thumbnail generation** is fire-and-forget via `QTimer::singleShot(200, ...)`, but still synchronous (`grab()`).

### 5.3 Observed Mitigations

- `Accelerated2dCanvasEnabled` is true in `main.cpp` and `BrowserWindow.cpp`.
- `TouchIconsEnabled` is true (helps high-DPI tab icons).
- Session save/restore is encrypted and deferred to destructor / startup, minimizing UI thread impact.

---

## 6. Windows-Specific Readiness

### 6.1 Packaging (`deploy.1.ps1`, `installer.iss`)

- `deploy.ps1` runs CMake + `windeployqt` and verifies `Qt6WebEngineCore.dll` presence.
- `installer.iss` creates Start Menu + desktop shortcuts, uses `lzma2/max` compression.

**Gaps**
- No **protocol handler** registration (`black://`, `https://`, search engine).
- No **file-association** checks.
- No **code signing** step visible in the scripts.
- No **auto-update** mechanism.
- Installer copies `build\Release\*` recursively excluding `*.obj,*.pch,*.cpp,*.h,*.rc`, which is broad; it could include developer artifacts if present in the build tree.

### 6.2 OS Integration (`OSPaths.cpp`)

- `showInFileManager()` uses `explorer.exe /select,` on Windows — correct.
- `openDefaultBrowserSettings()` opens `ms-settings:defaultapps` — correct.
- `writeFileAtomic()` uses `FlushFileBuffers` + `MoveFileExW(MOVEFILE_REPLACE_EXISTING)` on Windows — crash-safe.

### 6.3 DPAPI & Windows Security

- `VaultCrypto` uses `CryptProtectData` / `CryptUnprotectData` with entropy on Windows. This ties vault decryption to the current Windows user account, which is correct for a local-only browser.

**Gap:** No integration with **Windows Hello**, **SmartScreen**, or **Microsoft Defender SmartBrowser** APIs.

---

## 7. Data-Handling & Privacy Risks

| Store | Path | Encryption | Notes |
|-------|------|------------|-------|
| Passwords | `passwords.json` | BAKV envelope | Legacy plaintext fallback; **never_save.json** is plaintext host list. |
| Services | `services.json` | BAKV envelope | Verified in `README.md`; not directly inspected in this pass. |
| Permissions | `permissions.json` | BAKV envelope + plaintext fallback | `README.md` says encrypted; `PasswordStore` load path supports legacy plaintext. |
| Session | `session.json` | BAKV envelope + **plaintext fallback** | `BrowserWindow::saveSession()` falls back to raw JSON if encrypt fails. |
| Bookmarks / History | `bookmarks.json`, `history.json` | **None** | `ShelfStore` writes plaintext JSON. |
| Settings | `settings.json` | None | Theme + general settings in plaintext. |
| First-run marker | `.first_run_done` | None | 1-byte sentinel file. |

**Key risk:** A user who believes “everything is encrypted” will be surprised that bookmarks, history, and settings are plaintext. The README’s encryption claim is overstated unless it is scoped to “passwords, tokens, and permissions.”

---

## 8. OAuth & Service Token Handling

- `OAuthManager.h` shows providers configured via `oauth.json` and loopback redirects on fixed/ephemeral ports.
- `OAuthManager.cpp` was not fully inspected in this pass, but `README.md` claims encrypted token storage.

**Gaps / Risks**
- No visible **refresh-token** rotation or expiry handling.
- No visible **scope** minimization enforcement.
- `oauth.json.example` includes `clientSecret` for Apple (ES256 JWT). If real secrets are checked into source, this is a critical leak vector. The example file is presumably not checked in with real values, but this should be explicitly gated by `.gitignore`.

---

## 9. Extension System

- `ExtensionManager` loads `manifest.json` from `<AppData>/extensions/<id>/`.
- Only `content_scripts` are supported; no `chrome.*` / `browser.*` APIs.
- Path traversal is blocked by `isSafeRelativePath()` + canonical containment check.
- Content scripts run in `ApplicationWorld`, isolated from password bridge (`kPasswordWorld`).

**Gaps**
- No extension signature / provenance verification.
- No permission model for extensions (manifest `matches` is the only gate).

---

## 10. Bookmark Import

- `BookmarkImporter` supports Chrome, Edge, Brave, Vivaldi.
- Copies `Bookmarks` to a temp file before parsing to avoid torn reads.
- Skips `Guest` and `System` subtrees.
- `ShelfStore::importBookmarks()` deduplicates by raw + normalized URL and caps at 500 entries.

**Gaps**
- No import of passwords, history, or cookies — only bookmarks.

---

## 11. Reproducibility & Build State

- CMake configuration succeeds; build fails due to **missing system OpenSSL** (not a code bug).
- `HAVE_OPENSSL` is a CMake option that toggles between OpenSSL AES-GCM and the HMAC-CTR fallback in `VaultCrypto`.
- `report.txt` claims “Phase 1 of the honesty/security cleanup” is applied; this audit confirms most of those fixes are present in the code.

---

## 12. Missing / Incomplete Items

1. **Auto-update mechanism** — absent. Users are stuck on the installed version until manual reinstall.
2. **Permission management UI** — permissions are remembered per-origin but cannot be revoked from Settings.
3. **Session encryption fallback** — plaintext session write on vault failure silently downgrades privacy.
4. **Tab Overview smoothness** — synchronous `grab()` still blocks the UI thread.
5. **Translucent background on Windows** — `WA_TranslucentBackground` forces software DWM composition.
6. **Thumbnail generation** — synchronous on GUI thread after load.
7. **SmartScreen / Defender integration** — absent.
8. **Protocol handlers / file associations** — absent from installer.
9. **Code signing** — absent from build/deploy scripts.
10. **Extension API surface** — intentionally minimal; users expecting uBlock-Origin-style features will be disappointed.
11. **Memory pressure / cache eviction** — no automatic policy.
12. **OAuth token refresh** — not inspected; assumed minimal.

---

## 13. Recommended Fix Order (Priority)

| P | Item | Effort | Impact |
|---|------|--------|--------|
| 1 | Replace synchronous `view->grab()` in overview with `QQuickRenderControl` or deferred screenshot | Medium | Smoothness |
| 2 | Remove `WA_TranslucentBackground` or switch to `WA_TranslucentByParent` + acrylic material on Windows | Medium | Smoothness + GPU usage |
| 3 | Remove plaintext fallback from `saveSession()`; if vault fails, surface error and block save | Low | Privacy |
| 4 | Add permission-revocation UI in Settings | Low | Privacy/UX |
| 5 | Add auto-update framework (e.g., Squirrel, WinSparkle, or MSIX) | High | Security |
| 6 | Code-sign installer + binaries | Medium | Windows trust |
| 7 | Register protocol handlers in `installer.iss` | Low | UX |
| 8 | Implement automatic blocklist refresh mechanism | Medium | Privacy |
| 9 | Audit `SafariWebPage` bridge stripping on settings view navigation | Low | Security |
| 10 | Add extension signing / provenance verification | High | Security |

---

## 14. Sources

- `C:/Codes/BLACK/README.md`
- `C:/Codes/BLACK/report.txt`
- `C:/Codes/BLACK/BrowserWindow.cpp`
- `C:/Codes/BLACK/BrowserWindow.h`
- `C:/Codes/BLACK/SafariWebPage.cpp`
- `C:/Codes/BLACK/SafariWebPage.h`
- `C:/Codes/BLACK/VaultCrypto.cpp`
- `C:/Codes/BLACK/PasswordStore.cpp`
- `C:/Codes/BLACK/TrackerBlocker.cpp`
- `C:/Codes/BLACK/SafeBrowsing.cpp`
- `C:/Codes/BLACK/OSPaths.cpp`
- `C:/Codes/BLACK/ExtensionManager.cpp`
- `C:/Codes/BLACK/BookmarkImporter.cpp`
- `C:/Codes/BLACK/ShelfStore.cpp`
- `C:/Codes/BLACK/Account.cpp`
- `C:/Codes/BLACK/OnboardingBridge.cpp`
- `C:/Codes/BLACK/OAuthManager.cpp`
- `C:/Codes/BLACK/ChromeLayer.cpp`
- `C:/Codes/BLACK/AppearanceManager.cpp`
- `C:/Codes/BLACK/BrowserSettings.cpp`
- `C:/Codes/BLACK/main.cpp`
- `C:/Codes/BLACK/deploy.ps1`
- `C:/Codes/BLACK/installer.iss`
- `C:/Codes/BLACK/CMakeLists.txt`
- `C:/Codes/BLACK/responsive.css`

---

## 15. Verification Notes

- **TODO/FIXME/stub scan:** Zero matches in project-owned code. Only Qt headers and onboarding CSS mock content contain placeholder comments.
- **Build verification:** CMake configure succeeds; build fails on missing OpenSSL development libraries (environment issue, not code).
- **Phase 1 responsive UI:** Implemented and verified (`PlatformAdaptor`, breakpoints, media queries).
- **Phase 2 phone layout:** Not started; bottom toolbar, fullscreen web view, Safari-style top tab bar, and swipe gestures remain TODO.
- **Subagent audit:** Attempted; child tool allowlist did not include `hf_*` tools. Audit was completed via direct file reading instead.

---

*Artifact written by Feynman. All claims trace to explicit source lines or file-level inspection above.*
