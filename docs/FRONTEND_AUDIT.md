# BLACK Browser — Frontend (QWebChannel) Audit Report

Scope: all `qrc:/` HTML/JS/CSS pages and their bridge wiring against `main.cpp`, `BrowserWindow.cpp`, and the `*Bridge.h/.cpp` API surface. Read-only audit.

---

## 1. FRONTEND FLOW (QWebChannel architecture)

### 1.1 Channel wiring

There are **two independent web channels**:

**Main browser channel** (BrowserWindow.cpp:555-574) — attached to every tab via `view->setWebChannelObject(m_webChannel)` (BrowserWindow.cpp:1501) and to the settings dialog view (BrowserWindow.cpp:2120, 2134). Registered objects:

| Object | Class | Used by |
|---|---|---|
| `privacy` | TrackerBlocker::instance() | startpage, privacyreport |
| `theme` | SafariTheme::instance() | all pages |
| `browserSettings` | BrowserSettings::instance() | settings, startpage |
| `appearance` | AppearanceManager (uiLayout 0=Safari, 1=Chrome) | settings |
| `bookmarks` | ShelfStore("bookmarks.json") | bookmarks.html |
| `history` | ShelfStore("history.json") | history.html |
| `passwords` | PasswordStore (passwords.json) | settings |
| `extensions` | ExtensionManager::instance() | settings |
| `account` | Account::instance() | settings |
| `bookmarkImporter` | BookmarkImporter | *(no page uses it)* |
| `safeBrowsing` | SafeBrowsing::instance() | safebrowsing_warning |
| `settingsDialog` | SettingsDialogBridge | settings (registered only when dialog opens, :2134) |

**Onboarding channel** (main.cpp:323-334) — attached to the onboarding page via `page->setWebChannel(channel, QWebEngineScript::MainWorld)` before navigation. Objects: `onboardingBridge` (OnboardingBridge), `oauthManager` (OAuthManager), `appearance` (AppearanceManager), `theme` (SafariTheme::instance()), `bookmarkImporter` (unused by the page).

### 1.2 Page lifecycle

1. `main.cpp:334` injects the web channel; `main.cpp:346` loads `qrc:/onboarding_experience.html`. `qrc:///qtwebchannel/qwebchannel.js` is available at DOMContentLoaded — no transport race.
2. Onboarding page (onboarding_experience.html:745) builds `new QWebChannel(qt.webChannelTransport, cb)` and captures `ch.objects.onboardingBridge / oauthManager / appearance / theme`.
3. On completion, `completeOnboarding()` / `completeOnboardingWithProfile(profileJson)` / `finishAsGuest()` write the `.first_run_done` marker (main.cpp:348-361), hide the modal; main() then constructs BrowserWindow on the startpage.
4. Browser tabs are `qrc:/startpage_enhanced.html`; settings is a dedicated `qrc:/settings.html` dialog view; both receive the channel before navigation.
5. `applyUiLayoutToView` (BrowserWindow.cpp:3292-3304) injects `document.documentElement.dataset.uiLayout = "chrome"|"safari"` after each `loadFinished`; `chrome-theme.css` re-tokens page variables for the Chrome layout (light at :15-42, dark variants from :46).

### 1.3 Data flows

- **Tracker blocking**: pages read `privacy.websitesVisited / trackersBlockedToday / trackersBlockedLast30Days / mostContactedTracker / mostContactedTrackerSites / websitesContactedTrackers`; blocked navigations are redirected to `qrc:/safebrowsing_warning.html?url=<original>` (SafeBrowsing.cpp:275-276); **Continue Anyway** → `safeBrowsing.allow(host)` → redirect to the original URL (SafeBrowsing::isBlocked skips `m_allowed` hosts).
- **Bookmarks/History**: render `store.json`; row click → `window.location.href = row.dataset.url`; remove → `store.remove(url)`; Ctrl+D add is native.
- **Passwords**: settings lists `passwords.hosts()`, reads/edits via `passwordFor()/save()/remove()`, toggles never-save via `neverSaveJson/setNeverSave`.
- **Account**: settings reads `account.signedIn/name/email/avatar/authMethod`; onboarding writes via `saveAccount()/selectAccount()` and activates via `recordOAuthIdentity()` / `completeOnboardingWithProfile()`.
- **Layout/theme**: onboarding + settings segmented controls call `appearance.setUiLayout(0|1)` and `theme.setThemePreference(0|1|2)`; NOTIFY signals push updates back into every open page.

---

## 2. FLAWS (severity, file:line, impact)

### CRITICAL

**F1. `crash.html` is missing — qt_add_resources build break + broken crash recovery.**
- CMakeLists.txt:71 lists `crash.html` in `qt_add_resources(FILES ...)` but the file does not exist in the repo. `git log --all -- crash.html` is empty; commit `2e87ecf` added the CMakeLists entry without the file.
- Runtime reference: BrowserWindow.cpp:3342 (renderer crash → `setUrl(qrc:/crash.html?url=<percent-encoded>)`), :3820-3822 (session restore marks tabs crashed and navigates to crash.html); `kMaxRendererCrashReloads=2` at :216.
- Impact: the app cannot build as committed today, and the crash-recovery path — needed precisely when the renderer is already broken — lands on a nonexistent resource. The compiled `build/.qt/rcc/qrc_resources.cpp` is stale and also lacks `crash.html` and `vendor/three.min.js`.

### HIGH

**F2. OAuth profile silently lost when a signed-in user clicks "Skip Tour".**
- onboarding_experience.html:1799-1804 — Skip Tour (frame-14 `btn-skip`) calls `finishAsGuest()` → `Account::completeOnboarding()` only. It never activates the profile already saved to `accounts.json` via `saveAccount()` (frame-04) and never calls `recordOAuthIdentity()`.
- Contrast: **Launch** (btn-launch → `finishOnboardingReal()` → `completeOnboardingWithProfile(profileJson)`, :1791-1797) does activate it.
- Impact: user completes Google/Apple/Microsoft OAuth, skips the tour, and the browser opens **signed out** — `account.json` (what the browser actually reads) is never written. Two buttons next to each other with divergent identity behavior, invisible to the user.

**F3. `features.html` and `safebrowsing_warning.html` never load `chrome-theme.css`.**
- `grep 'link rel="stylesheet"'` returns **0** matches for features.html and safebrowsing_warning.html. Every other page links it: settings.html:451, startpage_enhanced.html:466, bookmarks.html:90, history.html:141, extensions.html:253, privacyreport.html:138. `chrome-theme.css` even ships a `[data-ui-layout="chrome"]` block targeting the features page that can never apply.
- Impact: in Classic Chrome layout these two pages keep the Safari palette while the surrounding chrome (toolbar/tabs/backdrop) is re-tokenized — visibly inconsistent, and the security-sensitive warning page doesn't match the active theme.

### MEDIUM

**F4. Startpage search mis-navigates bare domains → dead `qrc:/` links.**
- startpage_enhanced.html:693: `if (url.startsWith('http') || url.includes('.'))` → `window.location.href = url`, else `searchUrlFor(url)`.
- Typing `example.com` (no scheme) resolves relative to the `qrc://` origin → `qrc:/example.com`, a nonexistent resource (error page). Bare domains need `https://` prepended.

**F5. `javascript:`-scheme bookmark/history entries execute in the page context.**
- bookmarks.html:137 and history.html:228: `window.location.href = row.dataset.url` with no scheme whitelist.
- BookmarkImporter.cpp:119-126 imports the raw `url` node from Chrome's Bookmarks file with no scheme filtering; ShelfStore::add (ShelfStore.cpp) stores any string unchanged.
- Impact: a planted or imported `javascript:` bookmark executes in the `qrc://` origin, which hosts the webchannel objects (`passwords`, `bookmarks`, `browserSettings`, `account`, `extensions`) — a script URL in that context can drive the bridge.

**F6. Onboarding can dead-end if the WebChannel fails to initialize.**
- onboarding_experience.html:745 is guarded by `typeof QWebChannel !== "undefined"`; if the transport/script fails, `bridge` stays `null` and every `callBridge` no-ops — including the only exits: **Launch** (:1795), **Skip Tour** (:1799), and Escape → `cancel()` (:1806-1808). The user is trapped on the fullscreen first-run modal with no fallback. (Not reachable under a normal build; robustness issue.)

**F7. The import frame misrepresents what is actually migrated.**
- onboarding_experience.html:522-534 renders a transfer list (Passwords / Extensions / History / Open Tabs / Settings checkboxes), but the flow only calls `bridge.importBrowserBookmarks()` (:1331-1333). Only bookmarks are imported; the UI implies everything is.

### LOW

**F8. `bridge.onFrameChanged` is dead code — the method doesn't exist.**
- onboarding_experience.html:1048 checks `typeof bridge.onFrameChanged === "function"`; OnboardingBridge.h/.cpp define no such method. Guarded (harmless) but the `setInterval` call-site at :1046-1050 will never act.

**F9. `disableNext(id)` defined but never called.** — onboarding_experience.html:1816.

**F10. `qrc:/startpage_enhanced.html#reading` navigation has no anchor target.**
- BrowserWindow.cpp:1216 (Reading List shortcut) navigates to `...#reading`, but startpage_enhanced.html contains no element with `id="reading"` — scrolls nowhere.

**F11. `renderAccountList` interpolates a user-controlled char via innerHTML.**
- onboarding_experience.html:1258: `row.innerHTML = '<div class="avatar">' + initial + ...` with `initial = (acc.name || acc.email || "?").charAt(0)`. A display name starting with `<` injects markup. Cosmetic; should use `textContent`.

**F12. Startpage "Favourites" is hardcoded, not backed by the `bookmarks` bridge.**
- startpage_enhanced.html:513-554 — static grid (Apple, YouTube, X, GitHub, Gmail, Drive, Google). Label implies live favourites; nothing reads `channel.objects.bookmarks`. Feature gap / misleading UI.

**F13. `bookmarkImporter` is registered on both channels but unused by any page.**
- BrowserWindow.cpp:573 and main.cpp (onboarding channel): no HTML references `channel.objects.bookmarkImporter`; onboarding uses `bridge.importBrowserBookmarks()` instead. Dead registration.

**F14. privacyreport.html parses tracker JSON without a try/catch.**
- privacyreport.html:171 `JSON.parse(...)` in `renderList`; malformed `privacy.trackerBreakdownJson` would break the render chain. Low risk today.

**F15. Settings sidebar relies on the implicit global `event`.**
- settings.html:855 `if (event && event.currentTarget)` — reads `window.event`; works inside real DOM events and short-circuits safely, but is fragile if `switchTab` is ever invoked outside an event handler.

---

## 3. CONSISTENCY CHECK (pages vs registered bridges)

| Page | Channel objects referenced | Exists on channel | Mismatch |
|---|---|---|---|
| onboarding_experience.html | onboardingBridge, oauthManager, appearance, theme | ✓ (main.cpp:323-334) | — |
| settings.html | settingsDialog, theme, appearance, passwords, account, extensions | ✓ | — |
| startpage_enhanced.html | privacy, theme, browserSettings | ✓ | — |
| bookmarks.html | store (bookmarks), theme | ✓ | — |
| history.html | store (history), theme | ✓ | — |
| extensions.html | theme | ✓ | — |
| privacyreport.html | privacy, theme | ✓ | — |
| features.html | theme | ✓ | missing chrome-theme.css (F3) |
| safebrowsing_warning.html | safeBrowsing, theme | ✓ | missing chrome-theme.css (F3) |

Method-level verification:
- **OnboardingBridge**: every `bridge.*` call in onboarding_experience.html resolves (`completeOnboarding`, `finishAsGuest`, `completeOnboardingWithProfile`, `cancel`, `scanBrowsersJson`, `importBrowserBookmarks`, `accountsJson`, `saveAccount`, `selectAccount`) — **except `onFrameChanged`** (F8).
- **SafariTheme**: `theme.themeScheme / themePreference / setThemePreference` match the Q_PROPERTYs + NOTIFY signals ✓.
- **AppearanceManager**: `appearance.uiLayout / setUiLayout` match, incl. 0/1 normalization ✓.
- **TrackerBlocker**: all `privacy.*` counters and `privacyChanged` match ✓.
- **BrowserSettings**: `searchEngine` + `settingsChanged` match ✓.
- **PasswordStore**: `hosts / passwordFor / save / remove / clearAll / neverSaveJson / setNeverSave / changed` match ✓.
- **Account**: `signedIn / name / email / avatar / authMethod / changed / signOut` match ✓.
- **ExtensionManager**: `json / changed / reload` match ✓.
- **SafeBrowsing**: `safeBrowsing.allow(host)` matches; `isBlocked` only gates http/https and honors `m_allowed` ✓.
- **OAuthManager**: provider keys (OAuthManager.cpp:69-104: google, apple, microsoft, github, slack, discord, drive, calendar, dropbox, notion) match the page's sign-in/service switch; `profile.provider` uses the key (:375) ✓.
- **SettingsDialogBridge** (`currentPageUrl / closeSettingsDialog / hideSettingsDialog / toggleSettingsMaximize`, BrowserWindow.cpp:140-176) is only reachable from the settings view, where it is registered (:2134) ✓.
- No mixed-content `http://` references found in any page (all https).

---

## Priority order for fixes

1. **F1** — restore `crash.html` (unblocks the build; un-breaks crash recovery).
2. **F2** — make Skip Tour behave like Launch when an OAuth profile exists.
3. **F3** — add `chrome-theme.css` link to the two missing pages.
4. **F4 / F5** — scheme handling in startpage search and bookmark/history navigation.
5. F6–F15 — robustness and hygiene fixes.
