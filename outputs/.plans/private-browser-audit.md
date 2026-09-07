# Audit Plan: private-browser-audit

## Audit Target
- **Repo/Project:** `C:/Codes/BLACK` — Private desktop browser project (Qt/C++ with HTML/CSS internal pages)
- **Slug:** `private-browser-audit`

## Scope
1. **Architecture & Build System**
   - Inspect `CMakeLists.txt`, build directory, dependency declarations
   - Verify OpenSSL linking, Qt module usage, platform-specific flags
2. **Core Browser Features**
   - Inspect `BrowserWindow.cpp/h`, `SafariWebView.cpp/h`, `ChromeLayer.cpp/h`
   - Check tab management, navigation, back/forward, load handling
3. **Privacy & Security Claims**
   - Inspect `TrackerBlocker.cpp/h`, `SafeBrowsing.cpp/h`, `VaultCrypto.cpp/h`, `PasswordStore.cpp/h`
   - Verify actual implementation vs. implied private-browser guarantees
4. **Responsive UI / PlatformAdaptor**
   - Inspect `PlatformAdaptor.cpp/h`, `responsive.css`, HTML pages
   - Validate phone/tablet/desktop breakpoints, touch support, safe-area handling
5. **Performance & Smoothness Readiness**
   - Check for threading, GPU acceleration flags, rendering pipeline hints
   - Look for process isolation, memory management, caching strategies
6. **Windows-Specific Experience**
   - Inspect high-DPI handling, touch/pen support, installer/deployment scripts
   - Check Windows API usage, shortcuts, integration

## Evidence Gathering (researcher)
- Read all primary source files listed above
- Check `README.md`, `docs/`, `report.txt` for stated goals/claims
- Scan for TODOs, stubs, placeholder code, or unimplemented features
- Identify external dependencies vs. bundled/shipped dependencies

## Verification (verifier)
- Cross-reference every claim in HTML/CSS/README against actual C++ implementation
- Flag mismatches, missing code paths, ambiguous defaults, reproduction risks
- Add inline citations to specific files and line ranges where possible

## Output
- Single artifact: `outputs/private-browser-audit.md`
