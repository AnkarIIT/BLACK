# BLACK Browser - Feature Implementation Plan

Based on analysis of Chrome v156 and Safari Browser APKs, here are the key features to implement.

## 🎯 Priority 1: Core Missing Features (High Impact)

### 1. Video Downloader (Multi-platform)
**From Safari:** Video_MainActivity supports FB, Instagram, Twitter, Twitter with M3U8/HLS
- **Implementation:** Add `VideoDownloader` class with platform-specific extractors
- **Key features:** M3U8/HLS parsing, audio/video separation, FFmpeg merging
- **Files to create:** `VideoDownloader.h/.cpp`, `VideoDownloaderService.h/.cpp`, `M3U8Parser.h/.cpp`

### 2. Advanced Download Manager
**From Safari:** DownloaderService with M3U8/HLS, audio/video separation, notifications
- **Implementation:** Enhance `DownloadManager` with M3U8/HLS support
- **Key features:** Foreground service, progress notifications, resume, audio/video merge
- **Files to modify:** `DownloadManager.h/.cpp`, add `M3U8Downloader.h/.cpp`

### 3. QR/Barcode Scanner
**From Safari:** ZXing integration via `CaptureActivity`
- **Implementation:** Add `QRScanner` using ZXing C++ port
- **Files to create:** `QRScanner.h/.cpp`, integrate ZXing C++

### 4. VPN Integration
**From Safari:** Full VPN service with country selection (`vpnmaster` package)
- **Implementation:** Add `VPNManager` with VpnService integration
- **Files to create:** `VPNManager.h/.cpp`, `VPNService.h/.cpp`

## 🎯 Priority 2: Enhanced Browser Features

### 5. Onboarding Flow
**From Safari:** Multi-step onboarding with profile creation, OAuth
- **Implementation:** Enhance `onboarding_experience.html` with profile creation

### 6. Search Engine Selection
**From Safari:** `SearchEngineSelectionActivity` with 20+ engines
- **Implementation:** Already in settings.html, enhance with more engines

### 5. Tab Groups (Chrome-style)
**From Chrome:** Tab groups with colors, titles, collapse/expand
- **Implementation:** Add `TabGroupManager` with UI in tab overview

### 6. Reading List (Chrome-style)
**From Chrome:** Save pages for offline reading
- **Implementation:** Add `ReadingListManager` with offline storage

### 7. Password Manager Enhancements
**From Chrome:** Biometric unlock, autofill, credential manager
- **Implementation:** Enhance `PasswordStore` with biometric, WebAuthn

### 6. Reading Mode
**From Safari/Chrome:** Reader mode with content extraction
- **Implementation:** Complete `ReaderMode` with Mozilla Readability port

## 🎯 Priority 3: Polish & Integration

### 7. Tab Search & Previews (Chrome-style)
- Tab search in overview, live thumbnails

### 8. Extensions API (Chrome-style)
- MV3 manifest, content scripts, declarativeNetRequest

### 7. Picture-in-Picture
- Video PiP with resize/move

### 8. Sync Foundation
- Bookmarks, history, passwords, settings sync

---

## Implementation Order

1. **Video Downloader** (highest user value)
2. **Advanced Download Manager** (complements video downloader)
3. **QR Scanner** (standalone, high value)
4. **Download Manager Enhancement** (core browser feature)
5. **VPN Integration** (differentiator)
6. **Tab Groups** (power user feature)
7. **Reading List** (productivity)
8. **Reading Mode** (content consumption)
9. **Password Manager + WebAuthn** (security)
9. **Sync** (cross-device)

---

## Technical Notes

### Libraries to Integrate
- **ZXing C++** - QR/Barcode scanning
- **FFmpeg** - Video/audio processing (M3U8, merge)
- **ZXing C++** - QR/Barcode
- **OpenSSL** - VPN crypto
- **AdblockPlus** - Already integrated, enhance

### Qt Modules Needed
- `QtMultimedia` - Video playback, PiP
- `QtNetwork` - VPN, downloads
- `QtWebEngine` - Already used
- `QtPositioning` - Location permissions

---

## File Structure to Create

```
src/
├── videodownloader/
│   ├── VideoDownloader.h/.cpp
│   ├── M3U8Parser.h/.cpp
│   ├── VideoExtractor.h/.cpp (FB, Insta, Twitter, etc.)
│   └── FFmpegWrapper.h/.cpp
├── downloadmanager/
│   ├── M3U8Downloader.h/.cpp
│   ├── DownloadService.h/.cpp (foreground service)
│   └── AudioVideoMerger.h/.cpp
├── qrscanner/
│   ├── QRScanner.h/.cpp
│   └── ZXingWrapper.h/.cpp
├── vpn/
│   ├── VPNManager.h/.cpp
│   ├── VPNService.h/.cpp
│   └── VPNConfig.h/.cpp
├── qrscanner/
│   ├── QRScanner.h/.cpp
│   └── ZXingWrapper.h/.cpp
├── tabgroups/
│   ├── TabGroupManager.h/.cpp
│   └── TabGroupUI.h/.cpp
├── readinglist/
│   ├── ReadingListManager.h/.cpp
│   └── OfflinePageSaver.h/.cpp
├── readermode/
│   ├── ReaderMode.h/.cpp (enhance existing)
│   └── ReadabilityExtractor.h/.cpp
├── passwordmanager/
│   ├── WebAuthnManager.h/.cpp (enhance)
│   ├── BiometricAuth.h/.cpp
│   └── CredentialManager.h/.cpp
└── sync/
    ├── SyncManager.h/.cpp (enhance)
    ├── SyncEngine.h/.cpp
    └── SyncCrypto.h/.cpp
```

---

## Next Steps

Start with **Video Downloader** as it provides the highest user value and differentiates BLACK from other browsers.