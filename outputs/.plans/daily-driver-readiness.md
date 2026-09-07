# BLACK Browser — Daily-Driver Readiness Plan

**Goal:** Transform BLACK from a credible private-browser shell into a browser that can
realistically replace Chrome, Firefox, or Zen as a daily driver.

**Basis:** `docs/FRONTEND_AUDIT.md`, `docs/BACKEND_AUDIT.md`,
`outputs/private-browser-audit.md`, `outputs/fixes_applied.md`, plus direct
inspection of `BrowserWindow.cpp`, `SafariWebPage.cpp`, `VaultCrypto.cpp`,
`ShelfStore.cpp`, `main.cpp`, `OAuthManager.cpp`, `CMakeLists.txt`,
`installer.iss`, `deploy.ps1`.

---

## Phase 0 — Unblock Everything (Week 1)

*Nothing below can ship until this phase is green. Estimated effort: 1–2 days.*

### 0.1 Restore `crash.html` (F1, CRITICAL)

**Why first:** The file is listed in `CMakeLists.txt:71` but missing from the repo.
`BrowserWindow.cpp:3342` navigates to `qrc:/crash.html` on renderer crash, and
session restore does the same at `:3820-3822`. Without this file the app cannot
build as-committed, and the crash-recovery path is dead.

**Tasks:**
1. Create `crash.html` as a minimal internal page matching the existing chrome:
   - Load `chrome-theme.css`.
   - Show renderer-crash icon/message, "Reload" button, and "Open in new window" link.
   - Accept `?url=<percent-encoded>` query param for the failed URL.
   - Wire to `safeBrowsing.allow(host)`-style bridge if needed, or plain JS that
     calls `browser.navigate(...)` via existing channel.
2. Add `crash.html` to `qt_add_resources(FILES ...)` in `CMakeLists.txt` (already
   listed — confirm the path resolves).
3. Re-run CMake configure + build to confirm the resource compiles.

**Verification:**
- `cmake --build .` succeeds without missing-resource warnings.
- `find . -name "qrc_resources.cpp" -exec grep -l crash.html {} \;` returns a hit.
- Crash a tab manually (`kill` the renderer or navigate to a crashing URL) and
  confirm the crash page appears.

---

### 0.2 Wire Tests Into the Root Build

**Why first:** `tests/` has its own `CMakeLists.txt` and 7 test files, but the
root `CMakeLists.txt` never includes it. `ctest` reports "No tests were found!!!"
— meaning every security fix is unguarded.

**Tasks:**
1. Add to root `CMakeLists.txt`, after the `BLACK` target definition:
   ```cmake
   add_subdirectory(tests)
   ```
2. Confirm `BLACK_Tests` links correctly against the main sources (the tests
   `CMakeLists.txt` already uses `${CMAKE_SOURCE_DIR}/../` paths; verify those
   resolve when built as a subdirectory).
3. Fix the test `CMakeLists.txt` include paths so they work from either the
   root or the `tests/` subdirectory (use `${CMAKE_CURRENT_SOURCE_DIR}/..` or
   a shared `target_include_directories` on the `BLACK` target itself).

**Verification:**
- `ctest --output-on-failure` discovers and runs all 7 test executables.
- All existing tests pass (`test_vaultcrypto`, `test_trackerblocker`,
  `test_passwordstore`, `test_oauthmanager`, `test_shelffstore`,
  `test_permissionsbridge`, `test_main`).

---

### 0.3 Get a Clean Build in This Environment

**Why first:** The current `build/` directory is stale (VS 2026 generator, no
Qt installed). We need a reproducible build before changing anything else.

**Tasks:**
1. Install Qt 6.8.0 with WebEngine (Qt installer or `aqtinstall`).
2. Delete `build/` and re-configure:
   ```powershell
   cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
   ```
3. Build Release and confirm `build/Release/BLACK.exe` exists.
4. Add a `BUILDING.md` with exact Qt version, generator, and `CMAKE_PREFIX_PATH`
   used, so other developers can reproduce.

**Verification:**
- `build/Release/BLACK.exe` runs and shows the first-run onboarding.
- `ctest` passes.

---

## Phase 1 — Fix Security/Privacy Gaps (Week 2)

*Estimated effort: 3–5 days. These are the issues that misrepresent the product
to users or create real data-loss risks.*

### 1.1 Encrypt Bookmarks & History, or Correct the README

**Current state:** `ShelfStore` writes plaintext JSON. The README claims
"Encrypted-at-rest everything." This is the single largest credibility gap for
a privacy browser.

**Decision needed (pick one):**

**Option A — Encrypt (preferred for a privacy browser):**
1. Add a `ShelfStore::encrypt()` / `ShelfStore::decrypt()` path that uses
   `VaultCrypto::encrypt()` / `VaultCrypto::decrypt()`.
2. Detect legacy plaintext files on load, re-encrypt on next save.
3. Add atomic write via `OSPaths::writeFileAtomic()` (already implemented).
4. Update `README.md` encryption table to reflect the new scope.

**Option B — Correct the README (faster, honest):**
1. Change "Encrypted-at-rest everything" to "Encrypted-at-rest: passwords,
   service tokens, permissions, session. Bookmarks and history are local JSON."
2. Add a roadmap item for encrypted bookmarks/history.

**Verification:**
- Inspect `bookmarks.json` and `history.json` on disk after saving entries.
- Confirm they are either BAKV-encrypted blobs (Option A) or that the README
  no longer claims they are encrypted (Option B).

---

### 1.2 Remove Plaintext Session Fallback

**Current state:** `BrowserWindow::saveSession()` falls back to raw JSON if
`VaultCrypto::encrypt()` returns empty. This means a broken vault silently
downgrades session privacy.

**Tasks:**
1. In `BrowserWindow::saveSession()`, remove the plaintext fallback.
2. If encrypt fails, surface a `QMessageBox::warning()` (similar to the
   permission-save fix in `fixes_applied.md`) and abort the save.
3. Add a test: mock `VaultCrypto::encrypt()` returning empty, assert that
   `session.json` is not written as plaintext.

**Verification:**
- Simulate vault failure (e.g., temporarily rename the master key file) and
  confirm the warning appears and no `session.json` is written.
- Test passes in `test_main.cpp` or a new `test_browserwindow.cpp`.

---

### 1.3 Add Permission Revocation UI

**Current state:** Permissions are remembered per-origin in `permissions.json`
but there is no UI to revoke them.

**Tasks:**
1. Add a "Permissions" section to `settings.html` that reads
   `permissions.json` via the `browserSettings` or a new bridge method.
2. List each `origin|type` entry with a "Remove" button.
3. Wire the remove button to a new `browserSettings.removePermission(origin, type)`
   slot in `BrowserSettings.cpp`.
4. Style it consistently in both Safari and Chrome layouts.

**Verification:**
- Grant a permission, then revoke it from Settings, then revisit the site and
  confirm the prompt reappears.

---

### 1.4 Verify Backend OAuth Fixes Are Actually Present

**Current state:** `BACKEND_AUDIT.md` claims M11/M12/M13 are "DONE", but the
FRONTEND_AUDIT does not re-verify them. Before shipping, confirm each fix is
in the current source.

**Tasks:**
1. Search `OAuthManager.cpp` for `QRandomGenerator::system()` — confirm it is
   used for PKCE `verifier` and `state` (M11).
2. Search for `m_server->disconnect()` — confirm the blanket disconnect that
   severed callbacks is gone (M12).
3. Search for callback path + state validation logic — confirm callbacks are
   rejected unless `path == "/callback"` AND `state == activeState` (M13).
4. If any are missing, apply the fixes from the audit and add tests.

**Verification:**
- Grep/read confirms each fix.
- Add unit tests for state mismatch, wrong-path callback, and stale callback
   rejection in `test_oauthmanager.cpp`.

---

## Phase 2 — Performance & Smoothness (Week 3)

*Estimated effort: 2–4 days.*

### 2.1 Replace Synchronous `view->grab()` in Tab Overview

**Current state:** `BrowserWindow.cpp` calls `safeView->grab().scaled(...)` on
the GUI thread. This blocks the render loop and stutters with many tabs.

**Tasks:**
1. Replace with a deferred/async screenshot mechanism. Options:
   - Use `QWebEngineView::grab()` on a worker thread via `QMetaObject::invokeMethod`
     with `Qt::QueuedConnection` and a `QFuture`/`QPromise` to keep the UI free.
   - Or render thumbnails lazily when the overview opens, using `QTimer::singleShot`
     with a per-tab delay so the UI paints first.
2. Add a placeholder thumbnail color/icon immediately, then swap in the real
   grab when it completes.

**Verification:**
- Open Tab Overview with 20+ tabs; confirm no visible freeze.
- Profile with Qt Creator's CPU sampler; GUI thread should not block on `grab()`.

---

### 2.2 Fix Windows Compositing Hitches

**Current state:** Two confirmed issues in `BrowserWindow.cpp`:
- `QGraphicsDropShadowEffect` on active tab forces software rasterization.
- `WA_TranslucentBackground` + `FramelessWindowHint` disables the native
  Windows opaque-compositing path.

**Tasks:**
1. Remove or replace `QGraphicsDropShadowEffect` with a CSS-like border or
   native Windows `DwmExtendFrameIntoClientArea` acrylic effect.
2. On Windows, replace `WA_TranslucentBackground` with
   `WA_TranslucentByParent` + a layered window approach, or remove translucency
   entirely and use a solid title bar with rounded corners via
   `SetWindowRgn` / DWM.
3. Add platform-conditional compilation so Linux/macOS keep their current
   translucency behavior.

**Verification:**
- Run on a Windows machine with integrated graphics; confirm GPU usage drops
  and window resize/drag feels native.
- Compare against baseline with `QGraphicsDropShadowEffect` present.

---

### 2.3 Add GPU and Memory Flags

**Tasks:**
1. In `main.cpp`, add Chromium flags before `QApplication` construction:
   - `--enable-gpu-rasterization`
   - `--ignore-gpu-blocklist`
   - `--enable-zero-copy` (if supported)
2. Add a memory-pressure handler:
   - Connect to `QWebEngineProfile::downloadRequested` or a timer to call
     `profile()->clearHttpCache()` when memory usage is high (query via
     OS API or Qt process info).

**Verification:**
- `about:flags` equivalent or `QWebEngine::defaultProfile()->httpCacheMaximumSize()`
  reflects the new settings.
- Scrolling-heavy pages feel smoother on low-end hardware.

---

## Phase 3 — Daily-Driver Infrastructure (Weeks 4–6)

*Estimated effort: 1–2 weeks. These are the things that separate a project from
a product.*

### 3.1 Auto-Update Framework

**Tasks:**
1. Evaluate and pick one:
   - **WinSparkle** (Windows, simple, INI-based updates) — lowest effort.
   - **Squirrel.Windows** (Windows, delta updates, NSIS alternative).
   - **MSIX** (Windows modern deployment, auto-update via Store or sideload).
2. Implement:
   - Check for updates on startup + manual "Check for Updates" in Settings.
   - Download and apply update with user consent.
   - Add update channel selection (stable / beta).
3. Add update server or GitHub Releases integration.

**Verification:**
- Publish a v1.0.1 with a version bump; confirm the app detects and installs
  the update.

---

### 3.2 Code Signing

**Tasks:**
1. Obtain an EV or standard code-signing certificate.
2. Add signing to `deploy.ps1`:
   - Sign `BLACK.exe` and all DLLs with `signtool sign`.
   - Sign the installer (`installer.iss` output) as well.
3. Add timestamp server URL (`http://timestamp.digicert.com` or equivalent).

**Verification:**
- `signtool verify /pa BLACK.exe` returns "Successfully verified".
- Windows SmartScreen does not block the installer on a fresh machine.

---

### 3.3 Protocol Handlers & File Associations

**Tasks:**
1. In `installer.iss`, add registry entries for:
   - `http` / `https` protocol handlers.
   - `.html` / `.htm` file association (optional, since the engine handles them).
2. Add a "Set as default browser" button in Settings that opens
   `ms-settings:defaultapps` (already used in `OSPaths.cpp`).
3. Add a `black://` internal protocol for deep links (e.g., `black://settings`,
   `black://bookmarks`).

**Verification:**
- Clicking an `http` link in another app offers BLACK as a target.
- `black://settings` opens the Settings dialog.

---

### 3.4 Extension Content-Blocker API

**Why this is P1 instead of full WebExtensions:** Implementing a full
WebExtension host is a multi-month project. But the single most-requested
feature from privacy users is a content blocker (uBlock-style).

**Tasks:**
1. Define a minimal `manifest.json` schema for content blockers:
   - `content_scripts` with `matches` and `js` files (already partially supported).
   - Add a declarative rule format inspired by uBlock Origin's `$removeheader`,
     `$script`, `$image` etc., or simpler: a `blockedHosts` + `blockedPaths`
     JSON structure.
2. In `TrackerBlocker.cpp`, add a per-extension rule store and apply it in
   `interceptRequest()`.
3. Add an "Extensions" page that lists installed extensions with enable/disable
   toggles (partially exists; expand it).

**Verification:**
- Install a test extension that blocks `example.com/ads/*`.
- Confirm requests to that path are intercepted.

---

## Phase 4 — Competitive Parity & Polish (Weeks 7–10)

*Estimated effort: 2–3 weeks. These improve daily usability but are not blockers.*

### 4.1 DevTools / Web Inspector

**Tasks:**
1. Enable `QWebEngine::defaultProfile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache)`.
2. Add a DevTools toggle in settings or a right-click context menu that calls
   `view->page()->setDevToolsPage(...)`.
3. Persist DevTools dock state and window geometry.

---

### 4.2 Reader Mode

**Tasks:**
1. Add a Reader Mode toggle in the toolbar (Safari-style).
2. Use a simple readability algorithm (e.g., Mozilla's `Readability.js` port)
   injected into the page via `ApplicationWorld`.
3. Render the cleaned content in a `qrc:/reader.html` overlay.

---

### 4.3 WebAuthn / Passkeys

**Tasks:**
1. Enable `QWebEngineProfile::setHttpUserAgent` with a modern UA string that
   includes "WebAuthn".
2. Investigate Qt WebEngine's WebAuthn support (Chromium-backed; may work
   out-of-the-box if the platform has a biometric backend).
3. Add a "Passkeys" section in Settings listing saved credentials.

---

### 4.4 PiP, Translation, Per-Site Settings

**Tasks:**
- PiP: expose via `QWebEnginePage::requestedVideoAction` or inject a PiP
  button via `ApplicationWorld` script.
- Translation: inject a Google Translate or LibreTranslate iframe.
- Per-site settings: extend `permissions.json` schema to include zoom,
  content-blocking overrides, and user-agent spoofing.

---

### 4.5 Mobile / Cross-Platform

**Current state:** Qt WebEngine Widgets is desktop-only. The `CMakeLists.txt`
explicitly blocks Android and iOS.

**Long-term path (separate effort):**
- **Android:** Port to Qt Quick + Qt WebEngine Quick. This is a partial
  rewrite; estimate 2–3 months.
- **iOS:** Use WKWebView via a Qt iOS plugin or native Swift wrapper.
  This is a separate codebase; estimate 3–4 months.

**Decision:** Document this as a v2.0 goal. The desktop browser must stand on
its own before mobile matters.

---

## Summary: Execution Order

| Phase | Focus | Duration | Blocker? |
|-------|-------|----------|----------|
| 0 | Unblock build + tests | 1–2 days | Yes — everything depends on this |
| 1 | Security/privacy gaps | 3–5 days | Yes — ships with these broken |
| 2 | Performance/smoothness | 2–4 days | No — but users notice immediately |
| 3 | Auto-update, signing, protocols | 2–3 weeks | Yes — required for distribution |
| 4 | DevTools, reader, WebAuthn, PiP | 2–3 weeks | No — competitive parity |

**Minimum viable path to "can I use this daily?":**
- Complete Phases 0, 1, and 3.
- Accept that extensions will remain minimal until Phase 3.4 is done.
- Accept that mobile is out of scope for v1.0.

---

## Verification Gate Before Each Phase Merge

Each phase should produce:
1. **Green build** on at least one platform (Windows + Linux preferred).
2. **Green test suite** (`ctest --output-on-failure` passes).
3. **Updated audit artifact** (`docs/` or `outputs/`) listing what changed and
   what remains open.
4. **No new FRONTEND_AUDIT / BACKEND_AUDIT findings** introduced by the fix.

---

*Artifact written by Feynman. All phase items trace to specific findings in
`docs/FRONTEND_AUDIT.md`, `docs/BACKEND_AUDIT.md`,
`outputs/private-browser-audit.md`, and `outputs/fixes_applied.md`.*
