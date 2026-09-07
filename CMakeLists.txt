cmake_minimum_required(VERSION 3.16)
project(BLACK VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Mobile platforms ─────────────────────────────────────────────────────────
# BLACK is built on Qt WebEngine (Widgets), which is available for desktop
# (Windows/macOS/Linux) only:
#   * Android: Qt's WebEngine Widgets module is not supported on Android; the
#     platform uses a separate, incomplete WebEngine (Quick) stack. A full
#     rewrite in Qt Quick/QML would be required for a native Android build.
#   * iOS: Qt WebEngine is not shipped for iOS at all (Safari/WKWebView only).
# Fail loudly here rather than fail confusingly deep in the configure step.
if(ANDROID)
    message(FATAL_ERROR
        "BLACK cannot be built for Android as-is: Qt WebEngine (Widgets) used by "
        "this project is not supported on Android. Porting to Qt Quick/QML is "
        "required for a native Android build.")
elseif(IOS)
    message(FATAL_ERROR
        "BLACK cannot be built for iOS: Qt WebEngine is not available on iOS. "
        "An iOS port would need a native WKWebView rendering layer.")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Svg WebChannel Network)

# OpenSSL is only required on non-Windows for AES-GCM in VaultCrypto
# On Windows, DPAPI is used which doesn't require OpenSSL
if(WIN32)
    find_package(OpenSSL QUIET)
else()
    find_package(OpenSSL REQUIRED)
endif()

# macOS Keychain (Security framework) for VaultCrypto master key storage
if(APPLE)
    find_library(SECURITY Security)
    if(NOT SECURITY)
        message(FATAL_ERROR "Security framework not found on macOS")
    endif()
endif()

# Linux libsecret for VaultCrypto master key storage
if(LINUX)
    find_library(SECRET secret)
    if(NOT SECRET)
        message(WARNING "libsecret not found on Linux, falling back to file-based master key storage")
    endif()
endif()

qt_add_executable(BLACK
    main.cpp
    BrowserWindow.cpp
    BrowserWindow.h
    SafariWebView.cpp
    SafariWebView.h
    SafariWebPage.cpp
    SafariWebPage.h
    SafariTheme.cpp
    SafariTheme.h
    BrowserSettings.cpp
    BrowserSettings.h
    TrackerBlocker.cpp
    TrackerBlocker.h
    ShelfStore.cpp
    ShelfStore.h
    PasswordStore.cpp
    PasswordStore.h
    VaultCrypto.cpp
    VaultCrypto.h
    ExtensionManager.cpp
    ExtensionManager.h
    Account.cpp
    Account.h
    SafeBrowsing.cpp
    SafeBrowsing.h
    OSPaths.cpp
    OSPaths.h
    OnboardingBridge.h
    OnboardingBridge.cpp
    AppearanceManager.h
    AppearanceManager.cpp
    BookmarkImporter.cpp
    BookmarkImporter.h
    OAuthManager.cpp
    OAuthManager.h
    ChromeLayer.cpp
    ChromeLayer.h
    ChromeTheme.cpp
    ChromeTheme.h
    PlatformAdaptor.cpp
    PlatformAdaptor.h
    PermissionsBridge.cpp
    PermissionsBridge.h
    DownloadManager.cpp
    DownloadManager.h
    TabManager.cpp
    TabManager.h
    UpdateChecker.cpp
    UpdateChecker.h
)

qt_add_resources(BLACK "resources" PREFIX "/" BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        onboarding_experience.html
        vendor/three.min.js
        vendor/gsap.min.js
        startpage_enhanced.html
        bookmarks.html
        history.html
        settings.html
        features.html
        privacyreport.html
        safebrowsing_warning.html
        crash.html
        chrome-theme.css
        responsive.css
        app.png
    )

target_link_libraries(BLACK PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Svg
    Qt6::WebChannel
    Qt6::Network
)

if(WIN32)
    target_sources(BLACK PRIVATE resources.rc)
    target_link_libraries(BLACK PRIVATE crypt32 dwmapi)
elseif(APPLE)
    target_link_libraries(BLACK PRIVATE OpenSSL::SSL OpenSSL::Crypto ${SECURITY})
    target_compile_definitions(BLACK PRIVATE HAVE_OPENSSL)
elseif(LINUX)
    target_link_libraries(BLACK PRIVATE OpenSSL::SSL OpenSSL::Crypto ${SECRET})
    target_compile_definitions(BLACK PRIVATE HAVE_OPENSSL)
    if(SECRET)
        target_compile_definitions(BLACK PRIVATE HAVE_LIBSECRET)
    endif()
endif()

if(MSVC)
    target_compile_options(BLACK PRIVATE /W4)
else()
    target_compile_options(BLACK PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(BLACK PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "BLACK"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.black.browser"
)

add_subdirectory(tests)
