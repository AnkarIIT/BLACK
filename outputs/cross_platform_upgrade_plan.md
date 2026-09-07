# BLACK Browser — Cross-Platform Upgrade Plan

**Goal:** Make BLACK work seamlessly across **mobile, tablet, and desktop** — like Chrome/Edge/Safari do today.  
**Current State:** Desktop-only Qt 6/C++17 app with hardcoded fixed sizes and desktop chrome (traffic lights, tab strip).  
**Target State:** Responsive, touch-friendly, platform-adaptive browser that runs on Windows, macOS, Linux, Android, and iOS (or at minimum Android + desktop).

---

## 1. Current State Assessment

### 1.1 What Works Today
| Component | Status |
|-----------|--------|
| Desktop UI (Safari/Chrome layouts) | ✅ Working |
| Tab management | ✅ Working |
| Password vault | ✅ Working (encrypted) |
| Tracker blocking | ✅ Working |
| Extensions (content scripts) | ✅ Working |
| Session restore | ✅ Working |
| Internal HTML pages | ✅ Working (desktop CSS) |

### 1.2 What Blocks Mobile/Tablet
| Blocker | Why |
|---------|-----|
| `setMinimumSize(800, 500)` | Phones are 360–414px wide |
| Fixed-size buttons (12–46px) | Too small for touch (need 44–48px+) |
| Desktop-only chrome | Traffic lights, tab strip, titlebar controls |
| No responsive CSS | No `@media` queries for small screens |
| No touch gesture support | No swipe, pinch, long-press handling |
| No platform detection | Same UI everywhere |
| No mobile build target | CMake only builds desktop executable |
| `QMainWindow` centric | Mobile uses `QApplication` + `QStackedWidget` typically |

---

## 2. Architecture Changes

### 2.1 New Platform Abstraction Layer

Create `PlatformAdaptor` — a singleton that detects the device type and exposes the right UI mode.

```
PlatformAdaptor
├── detectFormFactor() → Desktop | Tablet | Phone
├── isTouchPrimary() → bool
├── recommendedMinimumSize() → QSize
├── chromeMode() → Full | Compact | None
├── tabBarPosition() → Top | Bottom
├── sidebarBehavior() → Floating | Overlay | BottomSheet
└── touchTargetSize() → int (44 for touch, 22 for mouse)
```

**Files to create:**
- `PlatformAdaptor.h`
- `PlatformAdaptor.cpp`

**Detection logic:**
```cpp
QSize screen = QGuiApplication::primaryScreen()->size();
float diagonal = sqrt(pow(screen.width(), 2) + pow(screen.height(), 2)) / 
                 QGuiApplication::primaryScreen()->physicalDotsPerInch();

if (diagonal < 6.0f) return Phone;        // < 6" diagonal
if (diagonal < 10.0f) return Tablet;      // 6"-10"
return Desktop;                            // 10"+ or mouse detected
```

### 2.2 Responsive Window Manager

Replace hardcoded `QMainWindow` with a `BrowserWindowBase` that adapts its chrome.

```
BrowserWindowBase
├── DesktopMode
│   ├── Traffic lights / titlebar
│   ├── Top tab strip
│   ├── Floating sidebar
│   └── URL bar with full chrome
├── TabletMode
│   ├── Compact top bar (no traffic lights)
│   ├── Bottom tab strip (thumb-friendly)
│   ├── Overlay sidebar
│   └── URL bar with larger touch targets
└── PhoneMode
    ├── No titlebar chrome
    ├── Bottom tab bar (like mobile Safari)
    ├── Fullscreen web view
    ├── Bottom sheet sidebar
    └── URL bar integrated into toolbar
```

---

## 3. UI Changes (High Priority)

### 3.1 Remove Hardcoded Minimum Size

**Current:**
```cpp
setMinimumSize(800, 500);
```

**New:**
```cpp
// Desktop
setMinimumSize(QSize(800, 500));
// Tablet
setMinimumSize(QSize(600, 400));
// Phone
setMinimumSize(QSize(320, 400));  // allow portrait phone widths
```

### 3.2 Responsive Sizing System

Replace all `setFixedSize()` with platform-aware sizing:

| Element | Current | Desktop | Tablet | Phone |
|---------|---------|---------|--------|-------|
| Traffic lights | 12px | 12px | — | — |
| Window controls | 46px | 46px | 48px | — |
| URL bar icons | 18–22px | 18px | 22px | 28px |
| Tab height | 28–32px | 32px | 40px | 48px |
| Buttons | 22–26px | 24px | 28px | 44px |
| Sidebar items | 18px icon | 18px | 22px | 28px |

### 3.3 New Phone Layout

```
┌─────────────────────────┐
│ [←]  [url bar...]  [⋯]  │ ← Compact top toolbar (48px)
├─────────────────────────┤
│                         │
│                         │
│    Web Content          │ ← Fullscreen WebEngineView
│                         │
│                         │
├─────────────────────────┤
│ [Tabs] [Search] [↩] [⋯]│ ← Bottom tab bar (56px)
└─────────────────────────┘
```

### 3.4 New Tablet Layout

```
┌─────────────────────────┐
│ [←]  [url bar...]  [⋯]  │ ← Compact top toolbar (52px)
├────┬────────────────────┤
│    │                    │
│ S  │   Web Content      │ ← Sidebar as overlay (swipe)
│ i  │                    │
│ d  │                    │
│ e  │                    │
│ b  │                    │
│ a  │                    │
│ r  │                    │
├────┴────────────────────┤
│     [Tab Strip]         │ ← Top tab strip (40px)
└─────────────────────────┘
```

---

## 4. Touch & Gesture Support

### 4.1 New Gesture Handler

Create `GestureHandler` to manage touch interactions.

```cpp
class GestureHandler : public QObject {
    Q_OBJECT
public:
    void attachToView(SafariWebView *view);
    
private slots:
    void onLongPress(const QPoint &pos);
    void onSwipeLeft();
    void onSwipeRight();
    void onPinchZoom(qreal factor);
    void onTwoFingerSwipeDown();
    
private:
    enum Gesture { None, Swipe, Pinch, LongPress };
};
```

**Gestures to implement:**
| Gesture | Action |
|---------|--------|
| Swipe left/right on toolbar | Navigate back/forward |
| Swipe up from bottom | Show sidebar/tabs |
| Swipe down on page | Refresh or show address bar |
| Long press link | Context menu |
| Pinch | Zoom (WebEngine handles, but need UI feedback) |
| Two-finger swipe down | Show notifications/permissions |

### 4.2 Touch-Optimized Context Menus

Replace desktop `QMenu` with `BottomSheetMenu` on touch devices.

```cpp
class BottomSheetMenu : public QDialog {
    // Slides up from bottom
    // Large touch targets (48px+)
    // Swipe down to dismiss
};
```

---

## 5. Internal Pages Responsive CSS

### 5.1 Current Problem
All internal HTML pages use desktop-only CSS. No `@media` queries.

### 5.2 Required Changes

Add to every internal page:

```css
/* Desktop (default) */
.container { max-width: 960px; padding: 48px 32px; }
.sidebar { width: 280px; position: fixed; }

/* Tablet */
@media (max-width: 1024px) {
    .container { max-width: 100%; padding: 32px 24px; }
    .sidebar { width: 260px; }
}

/* Phone */
@media (max-width: 640px) {
    .container { padding: 16px; }
    .sidebar { 
        width: 100%; 
        position: fixed; 
        bottom: 0; 
        top: auto;
        height: 60vh;
        border-radius: 16px 16px 0 0;
    }
    .quick-links { grid-template-columns: repeat(3, 1fr); }
    .section-title { font-size: 16px; }
}
```

**Pages to update:**
- `bookmarks.html`
- `history.html`
- `settings.html`
- `startpage_enhanced.html`
- `extensions.html`
- `privacyreport.html`
- `safebrowsing_warning.html`
- `crash.html`
- `onboarding_experience.html`

### 5.3 Global CSS Variables

Create `responsive.css`:

```css
:root {
    --touch-target: 44px;
    --toolbar-height: 48px;
    --tab-height: 40px;
    --bottom-bar-height: 56px;
    --sidebar-width: 280px;
    --spacing-unit: 8px;
}

@media (max-width: 640px) {
    :root {
        --touch-target: 48px;
        --toolbar-height: 52px;
        --tab-height: 48px;
        --bottom-bar-height: 60px;
        --sidebar-width: 100%;
    }
}
```

---

## 6. Code Refactoring

### 6.1 BrowserWindow Splitting

Current `BrowserWindow.cpp` is 4000+ lines. Split into:

```
BrowserWindow.h/cpp              → Main window, delegates to mode-specific classes
├── DesktopChrome.h/cpp          → Traffic lights, titlebar, menu bar
├── TabletChrome.h/cpp           → Compact toolbar, overlay sidebar
├── PhoneChrome.h/cpp            → Bottom toolbar, fullscreen web view
├── TabManager.h/cpp             → Tab strip management (position-aware)
├── NavigationBar.h/cpp          → URL bar, buttons (adapts to platform)
├── SidebarController.h/cpp      → Sidebar behavior per platform
└── GestureHandler.h/cpp         → Touch gestures
```

### 6.2 ChromeLayer Refactoring

`ChromeLayer` currently handles Safari↔Chrome toggle. Extend to handle platform modes:

```cpp
class ChromeLayer {
public:
    enum Mode { SafariDesktop, ChromeDesktop, Tablet, Phone };
    void setMode(Mode mode);
    void setChromeMode(bool chrome);  // existing, for desktop only
    
private:
    Mode m_mode;
    void applyDesktopChrome();
    void applyTabletChrome();
    void applyPhoneChrome();
};
```

### 6.3 Settings Dialog Adaptation

Current: Fixed `QDialog` with tabs.  
New: Platform-aware:

- **Desktop:** Side-by-side tabs in dialog
- **Tablet:** Fullscreen dialog with bottom navigation
- **Phone:** Fullscreen with top tabs

---

## 7. Build System Changes

### 7.1 CMake Multi-Platform Targets

Current CMake only builds desktop executable. Add:

```cmake
# Desktop (existing)
qt_add_executable(BLACK ...)

# Android
if(ANDROID)
    qt_add_executable(BLACK Android
        ...
    )
    qt_android_add_apk(BLACK ...)
endif()

# iOS
if(IOS)
    qt_add_executable(BLACK iOS
        ...
    )
    set_target_properties(BLACK PROPERTIES
        MACOSX_BUNDLE ON
        IOS_BUNDLE_IDENTIFIER "com.black.browser"
    )
endif()
```

### 7.2 Platform-Specific Resources

```
resources/
├── desktop/
│   ├── icons/
│   └── chrome/
├── tablet/
│   ├── icons/
│   └── chrome/
└── phone/
    ├── icons/
    └── chrome/
```

Load at runtime based on `PlatformAdaptor::formFactor()`.

---

## 8. Implementation Phases

### Phase 1: Foundation (Week 1-2)
| Task | Effort | Risk |
|------|--------|------|
| Create `PlatformAdaptor` | 2 days | Low |
| Remove hardcoded minimum size | 1 day | Low |
| Replace critical `setFixedSize()` with platform-aware sizing | 3 days | Medium |
| Add responsive CSS framework (`responsive.css`) | 2 days | Low |

**Deliverable:** App runs at any window size, no more hardcoded 800px minimum.

### Phase 2: Phone Layout (Week 3-4)
| Task | Effort | Risk |
|------|--------|------|
| Design phone chrome (bottom toolbar + fullscreen web view) | 3 days | Medium |
| Implement `PhoneChrome` class | 4 days | High |
| Implement bottom tab bar | 3 days | Medium |
| Implement fullscreen web view with URL bar reveal | 3 days | High |
| Add swipe gestures (back/forward, sidebar) | 3 days | Medium |

**Deliverable:** Phone-shaped window (320–414px wide) works with touch-friendly UI.

### Phase 3: Tablet Layout (Week 5-6)
| Task | Effort | Risk |
|------|--------|------|
| Design tablet chrome (compact top bar + overlay sidebar) | 2 days | Medium |
| Implement `TabletChrome` class | 3 days | Medium |
| Overlay sidebar with swipe gesture | 2 days | Medium |
| Adaptive tab strip (top, compact) | 2 days | Low |

**Deliverable:** Tablet-shaped window (600–1024px wide) with overlay sidebar.

### Phase 4: Desktop Polish (Week 7)
| Task | Effort | Risk |
|------|--------|------|
| Keep existing desktop mode working | 2 days | Low |
| Ensure Safari↔Chrome toggle still works | 1 day | Low |
| Test all window sizes | 2 days | Low |

### Phase 5: Touch & Gestures (Week 8-9)
| Task | Effort | Risk |
|------|--------|------|
| Implement `GestureHandler` | 4 days | High |
| Long-press context menus | 2 days | Medium |
| Swipe navigation | 2 days | Medium |
| Bottom sheet menus | 2 days | Medium |

### Phase 6: Internal Pages (Week 10-11)
| Task | Effort | Risk |
|------|--------|------|
| Add responsive CSS to all 9 HTML pages | 5 days | Medium |
| Test settings, bookmarks, history on all sizes | 2 days | Low |

### Phase 7: Build & Deploy (Week 12)
| Task | Effort | Risk |
|------|--------|------|
| Add Android build target | 3 days | High |
| Add iOS build target (if needed) | 4 days | High |
| App icons and splash screens per platform | 2 days | Low |
| Store assets (Google Play, etc.) | 2 days | Low |

---

## 9. Risk Analysis

| Risk | Mitigation |
|------|------------|
| Qt WebEngine on mobile is heavy | Use Qt 6.8+ with optimized WebEngine; test memory early |
| Touch gestures conflict with web content | Use `QWebEngineView::gestureEvent()` and careful propagation |
| App size too large for mobile | Strip unused Qt modules; use Qt for WebAssembly? No, use native |
| Apple App Store rejection | Need proper privacy policy, no private APIs |
| Google Play policy | Need privacy policy, permissions justification |
| Performance on low-end phones | Profile early; consider lighter UI mode |

---

## 10. What This Does NOT Cover

| Item | Reason |
|------|--------|
| Full Chrome extension API | Out of scope; current content_scripts model sufficient for mobile |
| Cloud sync | Separate feature; local-first remains |
| iOS support initially | Apple restrictions make Qt WebEngine iOS deployment difficult; start with Android |
| WebAssembly target | Desktop-only; native mobile builds preferred |
| Fuchsia/other platforms | Future consideration |

---

## 11. Recommended Starting Point

**Start with Phase 1** — it's the foundation and has the highest ROI:

1. `PlatformAdaptor` — 2 days
2. Remove minimum size constraint — 1 day
3. Replace top 20 most critical `setFixedSize()` calls — 3 days
4. Add `responsive.css` — 2 days

This gets the app running in a resizable window from 320px to 3840px wide, which immediately validates the approach before committing to full phone/tablet chrome redesign.

---

## 12. Questions to Decide Before Starting

1. **Android first, or desktop responsive first?** (Recommended: desktop responsive first, Android second)
2. **iOS target?** (Qt WebEngine iOS is experimental — recommend skipping initially)
3. **Minimum supported phone width?** (Recommended: 320px for Android compatibility)
4. **Keep Safari/Chrome toggle on mobile?** (Recommended: no, use single mobile-optimized layout)
5. **Tab strip position on phone?** (Recommended: bottom, like Safari/Chrome mobile)

---

*Plan created: 2025-01-25*  
*Estimated total: 12 weeks, 1 engineer*  
*Risk level: Medium-High (mobile UI redesign is significant)*
