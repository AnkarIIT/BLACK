$files = @(
    ".gitignore",
    "Account.cpp",
    "AdvancedSecurity.cpp",
    "AdvancedSecurity.h",
    "AIAgentManager.cpp",
    "AIAgentManager.h",
    "AIManager.cpp",
    "AIManager.h",
    "BookmarkImporter.cpp",
    "BookmarkImporter.h",
    "BrowserSettings.cpp",
    "BrowserSettings.h",
    "BrowserWindow.cpp",
    "BrowserWindow.h",
    "ChromeLayer.cpp",
    "chrome-theme.css",
    "CMakeLists.txt",
    "CollaborationManager.cpp",
    "CollaborationManager.h",
    "CookiePartition.cpp",
    "CookiePartition.h",
    "crash.html",
    "CrashReporter.cpp",
    "CrashReporter.h",
    "CrashReporting.h",
    "deploy.ps1",
    "DeveloperTools.cpp",
    "DeveloperTools.h",
    "DownloadManager.cpp",
    "DownloadManager.h",
    "ExtensionApi.cpp",
    "ExtensionApi.h",
    "ExtensionManager.cpp",
    "ExtensionManager.h",
    "FilterListParser.cpp",
    "FilterListParser.h",
    "FingerprintProtection.cpp",
    "FingerprintProtection.h",
    "I18nManager.cpp",
    "I18nManager.h",
    "IMPLEMENTATION_PLAN.md",
    "installer.iss",
    "MacOSWindowHelper.cpp",
    "MacOSWindowHelper.h",
    "main.cpp",
    "oauth.json.example",
    "OAuthManager.h",
    "OSPaths.cpp",
    "PerformanceManager.cpp",
    "PerformanceManager.h",
    "PermissionsBridge.cpp",
    "PermissionsBridge.h",
    "ProductivityManager.cpp",
    "ProductivityManager.h",
    "ProfileManager.cpp",
    "ProfileManager.h",
    "ReaderMode.cpp",
    "ReaderMode.h",
    "RewardsManager.cpp",
    "RewardsManager.h",
    "SAFARI_ENHANCEMENT.md",
    "SafariTheme.h",
    "SafariWebPage.cpp",
    "SafariWebPage.h",
    "SafeBrowsing.cpp",
    "SafeBrowsing.h",
    "Sandbox.cpp",
    "Sandbox.h",
    "SearchAffiliation.cpp",
    "SearchAffiliation.h",
    "settings.html",
    "ShelfStore.cpp",
    "ShelfStore.h",
    "startpage_enhanced.html",
    "SyncManager.cpp",
    "SyncManager.h",
    "TabController.cpp",
    "TabController.h",
    "TabGroupManager.cpp",
    "TabGroupManager.h",
    "TabManager.h",
    "tests/CMakeLists.txt",
    "tests/playwright/package-lock.json",
    "tests/playwright/playwright.config.ts",
    "tests/playwright/serve.mjs",
    "tests/playwright/tests/browser.spec.ts",
    "tests/playwright/tests/chrome-theme.spec.ts",
    "tests/playwright/tests/settings-api.spec.ts",
    "tests/test_accessibility.cpp",
    "tests/test_account.cpp",
    "tests/test_browserwindow.cpp",
    "tests/test_downloadmanager.cpp",
    "tests/test_extensionmanager.cpp",
    "tests/test_oauthmanager.cpp",
    "tests/test_passwordstore.cpp",
    "tests/test_safebrowsing.cpp",
    "tests/test_shelffstore.cpp",
    "tests/test_tab_thumbnail_download.cpp",
    "tests/test_videodownloader.cpp",
    "ToolbarController.h",
    "TrackerBlocker.cpp",
    "TrackerBlocker.h",
    "TranslationManager.cpp",
    "TranslationManager.h",
    "UpdateChecker.cpp",
    "UpdateChecker.h",
    "VaultCrypto.cpp",
    "VideoDownloader.cpp",
    "VideoDownloader.h",
    "VoiceInterface.cpp",
    "VoiceInterface.h",
    "WalletManager.cpp",
    "WalletManager.h",
    "WasmExtensionSupport.cpp",
    "WasmExtensionSupport.h",
    "WebAuthn.cpp",
    "WebAuthn.h",
    "WebRTCManager.cpp",
    "WebRTCManager.h"
)

$commitMessages = @{
    ".gitignore" = "chore: update gitignore"
    "Account.cpp" = "fix(account): fail-closed decrypt and OAuth host allowlist"
    "AdvancedSecurity.cpp" = "feat: add advanced security features"
    "AdvancedSecurity.h" = "feat: add advanced security features header"
    "AIAgentManager.cpp" = "feat: add AI agent manager"
    "AIAgentManager.h" = "feat: add AI agent manager header"
    "AIManager.cpp" = "feat: add AI manager"
    "AIManager.h" = "feat: add AI manager header"
    "BookmarkImporter.cpp" = "fix(bookmarkimporter): harden import parsing"
    "BookmarkImporter.h" = "fix(bookmarkimporter): harden import parsing header"
    "BrowserSettings.cpp" = "fix(browsersettings): minor fix"
    "BrowserSettings.h" = "fix(browsersettings): minor fix header"
    "BrowserWindow.cpp" = "feat(browserwindow): add tab API, accessibility, focus, WebEngine fixes"
    "BrowserWindow.h" = "feat(browserwindow): add public tab API declarations"
    "ChromeLayer.cpp" = "fix(chromelayer): minor fix"
    "chrome-theme.css" = "feat: add chrome theme CSS"
    "CMakeLists.txt" = "build: fix libsecret, enable_testing, test targets"
    "CollaborationManager.cpp" = "feat: add collaboration manager"
    "CollaborationManager.h" = "feat: add collaboration manager header"
    "CookiePartition.cpp" = "feat: add cookie partition"
    "CookiePartition.h" = "feat: add cookie partition header"
    "crash.html" = "feat: add crash page"
    "CrashReporter.cpp" = "feat: add crash reporter"
    "CrashReporter.h" = "feat: add crash reporter header"
    "CrashReporting.h" = "feat: add crash reporting"
    "deploy.ps1" = "ci: add Windows deploy script"
    "DeveloperTools.cpp" = "feat: add developer tools"
    "DeveloperTools.h" = "feat: add developer tools header"
    "DownloadManager.cpp" = "feat: add download manager"
    "DownloadManager.h" = "feat: add download manager header"
    "ExtensionApi.cpp" = "feat: add extension API"
    "ExtensionApi.h" = "feat: add extension API header"
    "ExtensionManager.cpp" = "feat: add extension manager"
    "ExtensionManager.h" = "feat: add extension manager header"
    "FilterListParser.cpp" = "feat: add filter list parser"
    "FilterListParser.h" = "feat: add filter list parser header"
    "FingerprintProtection.cpp" = "feat: add fingerprint protection"
    "FingerprintProtection.h" = "feat: add fingerprint protection header"
    "I18nManager.cpp" = "feat: add i18n manager"
    "I18nManager.h" = "feat: add i18n manager header"
    "IMPLEMENTATION_PLAN.md" = "docs: add implementation plan"
    "installer.iss" = "ci: add Windows installer script"
    "MacOSWindowHelper.cpp" = "feat: add macOS window helper"
    "MacOSWindowHelper.h" = "feat: add macOS window helper header"
    "main.cpp" = "fix(main): minor fix"
    "oauth.json.example" = "docs: add OAuth config example"
    "OAuthManager.h" = "fix(oauthmanager): minor fix header"
    "OSPaths.cpp" = "fix(ospaths): minor fix"
    "PerformanceManager.cpp" = "feat: add performance manager"
    "PerformanceManager.h" = "feat: add performance manager header"
    "PermissionsBridge.cpp" = "fix(permissionsbridge): various fixes"
    "PermissionsBridge.h" = "fix(permissionsbridge): minor fix header"
    "ProductivityManager.cpp" = "feat: add productivity manager"
    "ProductivityManager.h" = "feat: add productivity manager header"
    "ProfileManager.cpp" = "feat: add profile manager"
    "ProfileManager.h" = "feat: add profile manager header"
    "ReaderMode.cpp" = "feat: add reader mode"
    "ReaderMode.h" = "feat: add reader mode header"
    "RewardsManager.cpp" = "feat: add rewards manager"
    "RewardsManager.h" = "feat: add rewards manager header"
    "SAFARI_ENHANCEMENT.md" = "docs: add Safari enhancement notes"
    "SafariTheme.h" = "fix(safaritheme): minor fix"
    "SafariWebPage.cpp" = "fix(safariwebpage): minor fix"
    "SafariWebPage.h" = "fix(safariwebpage): minor fix header"
    "SafeBrowsing.cpp" = "feat: add safe browsing"
    "SafeBrowsing.h" = "feat: add safe browsing header"
    "Sandbox.cpp" = "feat: add sandbox"
    "Sandbox.h" = "feat: add sandbox header"
    "SearchAffiliation.cpp" = "feat: add search affiliation"
    "SearchAffiliation.h" = "feat: add search affiliation header"
    "settings.html" = "fix(settings): XSS fix, API bridge"
    "ShelfStore.cpp" = "fix(shelffstore): minor fix"
    "ShelfStore.h" = "fix(shelffstore): minor fix header"
    "startpage_enhanced.html" = "feat: add enhanced start page"
    "SyncManager.cpp" = "feat: add sync manager"
    "SyncManager.h" = "feat: add sync manager header"
    "TabController.cpp" = "feat: add tab controller"
    "TabController.h" = "feat: add tab controller header"
    "TabGroupManager.cpp" = "feat: add tab group manager"
    "TabGroupManager.h" = "feat: add tab group manager header"
    "TabManager.h" = "feat: add tab manager header"
    "tests/CMakeLists.txt" = "test: fix test registration, platform gating"
    "tests/playwright/package-lock.json" = "test: update playwright deps"
    "tests/playwright/playwright.config.ts" = "test: update playwright config"
    "tests/playwright/serve.mjs" = "test: update playwright test server"
    "tests/playwright/tests/browser.spec.ts" = "test: add browser tests"
    "tests/playwright/tests/chrome-theme.spec.ts" = "test: add chrome theme test"
    "tests/playwright/tests/settings-api.spec.ts" = "test: add settings API regression test"
    "tests/test_accessibility.cpp" = "test: fix accessibility tests"
    "tests/test_account.cpp" = "test: fix account test assertions"
    "tests/test_browserwindow.cpp" = "test: fix browser window tests"
    "tests/test_downloadmanager.cpp" = "test: fix download manager tests"
    "tests/test_extensionmanager.cpp" = "test: fix extension manager tests"
    "tests/test_oauthmanager.cpp" = "test: fix OAuth manager tests"
    "tests/test_passwordstore.cpp" = "test: fix password store tests"
    "tests/test_safebrowsing.cpp" = "test: fix safe browsing tests"
    "tests/test_shelffstore.cpp" = "test: fix shelf store tests"
    "tests/test_tab_thumbnail_download.cpp" = "test: fix tab thumbnail tests"
    "tests/test_videodownloader.cpp" = "test: add video downloader tests"
    "ToolbarController.h" = "feat: add toolbar controller header"
    "TrackerBlocker.cpp" = "feat: add tracker blocker"
    "TrackerBlocker.h" = "feat: add tracker blocker header"
    "TranslationManager.cpp" = "feat: add translation manager"
    "TranslationManager.h" = "feat: add translation manager header"
    "UpdateChecker.cpp" = "fix(windows): QTemporaryFile member, QCryptographicHash fix"
    "UpdateChecker.h" = "fix(windows): QTemporaryFile member header"
    "VaultCrypto.cpp" = "fix(linux): libsecret via find_path/find_library"
    "VideoDownloader.cpp" = "feat: add video downloader"
    "VideoDownloader.h" = "feat: add video downloader header"
    "VoiceInterface.cpp" = "feat: add voice interface"
    "VoiceInterface.h" = "feat: add voice interface header"
    "WalletManager.cpp" = "feat: add wallet manager"
    "WalletManager.h" = "feat: add wallet manager header"
    "WasmExtensionSupport.cpp" = "feat: add WASM extension support"
    "WasmExtensionSupport.h" = "feat: add WASM extension support header"
    "WebAuthn.cpp" = "feat: add WebAuthn"
    "WebAuthn.h" = "feat: add WebAuthn header"
    "WebRTCManager.cpp" = "feat: add WebRTC manager"
    "WebRTCManager.h" = "feat: add WebRTC manager header"
}

$targetHash = "756a9a0"

foreach ($file in $files) {
    Write-Host "Processing $file..."
    $tempFile = "$file.new"
    
    try {
        # Get the file from the target commit
        $cmd = "git show " + $targetHash + ":`"$file`""
        $content = Invoke-Expression $cmd
        $content | Out-File -FilePath $tempFile -Encoding utf8
        
        # Check if file exists in current repo
        if (Test-Path $file) {
            Remove-Item $file -Force
        }
        
        # Move new file into place
        Move-Item $tempFile $file -Force
        
        # Add and commit
        git add $file
        $msg = $commitMessages[$file]
        if (-not $msg) { $msg = "update $file" }
        git commit -m $msg
        Write-Host "  Committed: $msg"
    }
    catch {
        $errMsg = $_.Exception.Message
        Write-Error ("Failed on " + $file + ": " + $errMsg)
        # Clean up temp file if exists
        if (Test-Path $tempFile) { Remove-Item $tempFile -Force }
    }
}

Write-Host "All files processed!"