cmake_minimum_required(VERSION 3.16)
project(BLACK VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Configure version header
configure_file(version.h.in version.h @ONLY)

# ── Platform detection ───────────────────────────────────────────────────────
# Detect architecture for cross-compilation support
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(BLACK_ARM64 ON)
    message(STATUS "ARM64 architecture detected")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
    set(BLACK_X86_64 ON)
    message(STATUS "x86_64 architecture detected")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch32")
    set(BLACK_ARM32 ON)
    message(STATUS "ARM32 architecture detected")
else()
    message(WARNING "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

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

# Windows on ARM support
if(WIN32 AND BLACK_ARM64)
    message(STATUS "Windows on ARM (ARM64) detected - enabling ARM64 optimizations")
    add_compile_options(/favor:ARM64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DARM64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Svg WebChannel Network Positioning Quick QuickControls2 QuickTemplates2 Qml WebEngineQuick WebEngineQuickDelegatesQml)

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
    UpdateChecker.cpp
    UpdateChecker.h
    IWebEngine.h
    QtWebEngineBackend.cpp
    QtWebEngineBackend.h
    ExtensionApi.cpp
    ExtensionApi.h
    WebAuthn.cpp
    WebAuthn.h
    FilterListParser.cpp
    FilterListParser.h
    ReaderMode.cpp
    ReaderMode.h
    ProfileManager.cpp
    ProfileManager.h
    Sandbox.cpp
    Sandbox.h
    CrashReporter.cpp
    CrashReporter.h
    FingerprintProtection.cpp
    FingerprintProtection.h
    CookiePartition.cpp
    CookiePartition.h
    TranslationManager.cpp
    TranslationManager.h
    TabGroup.cpp
    TabGroup.h
    TabGroupManager.cpp
    TabGroupManager.h
    TabController.cpp
    TabController.h
    SidebarController.cpp
    SidebarController.h
    ToolbarController.cpp
    ToolbarController.h
    LayoutManager.cpp
    LayoutManager.h
    MemoryPressureManager.cpp
    MemoryPressureManager.h
    ExtensionAPI.cpp
    ExtensionAPI.h
    CrashReporting.cpp
    CrashReporter.h
    SyncCrypto.cpp
    SyncCrypto.h
    SyncEngine.cpp
    SyncEngine.h
    SyncManager.cpp
    SyncManager.h
    MacOSWindowHelper.cpp
    MacOSWindowHelper.h
    AIManager.cpp
    AIManager.h
    WalletManager.cpp
    WalletManager.h
    RewardsManager.cpp
    RewardsManager.h
    SearchAffiliation.cpp
    SearchAffiliation.h
    PerformanceManager.cpp
    PerformanceManager.h
    WasmExtensionSupport.cpp
    WasmExtensionSupport.h
    WebRTCManager.cpp
    WebRTCManager.h
    DeveloperTools.cpp
    DeveloperTools.h
    I18nManager.cpp
    I18nManager.h
    CollaborationManager.cpp
    CollaborationManager.h
AIAgentManager.cpp
    AIAgentManager.h
    VoiceInterface.cpp
    VoiceInterface.h
    ProductivityManager.cpp
    ProductivityManager.h
    RewardsManager.cpp
    RewardsManager.h
    WalletManager.cpp
    WalletManager.h
    SyncCrypto.cpp
    SyncCrypto.h
    SyncEngine.cpp
    SyncEngine.h
    SyncManager.cpp
    SyncManager.h
    AdvancedSecurity.cpp
    AdvancedSecurity.h
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

qt_add_resources(BLACK "qml" PREFIX "/" BASE "${CMAKE_CURRENT_SOURCE_DIR}/qml"
    FILES
        qml/MainWindow.qml
    )

target_link_libraries(BLACK PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Svg
    Qt6::WebChannel
    Qt6::Network
    Qt6::Positioning
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickTemplates2
    Qt6::Qml
)

target_include_directories(BLACK PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

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

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_FUZZING "Enable libFuzzer harnesses" OFF)

if(ENABLE_ASAN)
    if(MSVC)
        target_compile_options(BLACK PRIVATE /fsanitize=address)
        target_link_options(BLACK PRIVATE /fsanitize=address)
    else()
        target_compile_options(BLACK PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(BLACK PRIVATE -fsanitize=address)
    endif()
endif()

if(ENABLE_TSAN)
    if(NOT MSVC)
        target_compile_options(BLACK PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
        target_link_options(BLACK PRIVATE -fsanitize=thread)
    endif()
endif()

if(ENABLE_MSAN)
    if(NOT MSVC)
        target_compile_options(BLACK PRIVATE -fsanitize=memory -fno-omit-frame-pointer)
        target_link_options(BLACK PRIVATE -fsanitize=memory)
    endif()
endif()

if(ENABLE_UBSAN)
    if(MSVC)
        target_compile_options(BLACK PRIVATE /fsanitize=undefined)
    else()
        target_compile_options(BLACK PRIVATE -fsanitize=undefined -fno-sanitize-recover=all)
        target_link_options(BLACK PRIVATE -fsanitize=undefined)
    endif()
endif()

set_target_properties(BLACK PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "BLACK"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.black.browser"
)

add_subdirectory(tests)

if(ENABLE_FUZZING)
    if(NOT MSVC)
        # TrackerBlocker fuzzer
        add_executable(trackerblocker_fuzzer
            tests/fuzzer_trackerblocker.cpp
        )
        target_link_libraries(trackerblocker_fuzzer PRIVATE
            Qt6::Core
            TrackerBlocker
            libFuzzer
        )
        target_compile_options(trackerblocker_fuzzer PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
        target_link_options(trackerblocker_fuzzer PRIVATE -fsanitize=fuzzer,address)
        
        # SafeBrowsing fuzzer
        add_executable(safebrowsing_fuzzer
            tests/fuzzer_safebrowsing.cpp
        )
        target_link_libraries(safebrowsing_fuzzer PRIVATE
            Qt6::Core
            SafeBrowsing
            libFuzzer
        )
        target_compile_options(safebrowsing_fuzzer PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
        target_link_options(safebrowsing_fuzzer PRIVATE -fsanitize=fuzzer,address)
        
        # VaultCrypto fuzzer
        add_executable(vaultcrypto_fuzzer
            tests/fuzzer_vaultcrypto.cpp
        )
        target_link_libraries(vaultcrypto_fuzzer PRIVATE
            Qt6::Core
            VaultCrypto
            libFuzzer
        )
        target_compile_options(vaultcrypto_fuzzer PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
        target_link_options(vaultcrypto_fuzzer PRIVATE -fsanitize=fuzzer,address)
        
        # URL parser fuzzer
        add_executable(urlparser_fuzzer
            tests/fuzzer_urlparser.cpp
        )
        target_link_libraries(urlparser_fuzzer PRIVATE
            Qt6::Core
            libFuzzer
        )
        target_compile_options(urlparser_fuzzer PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
        target_link_options(urlparser_fuzzer PRIVATE -fsanitize=fuzzer,address)
    endif()
endif()

# QML-based UI for mobile/touch support
qt_add_executable(BLACK_QML
    qml/main.cpp
)

# Android build configuration
if(ANDROID)
    qt_add_executable(BLACK_ANDROID
        android/main.cpp
    )

    qt_add_resources(BLACK_ANDROID "qml" PREFIX "/" BASE "${CMAKE_CURRENT_SOURCE_DIR}/qml"
        FILES
            qml/android/MainView.qml
    )

    target_link_libraries(BLACK_ANDROID PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickTemplates2
        Qt6::Qml
        Qt6::WebEngineQuick
        Qt6::WebChannel
        Qt6::Network
        Qt6::Positioning
    )

    if(MSVC)
        target_compile_options(BLACK_ANDROID PRIVATE /W4)
    else()
        target_compile_options(BLACK_ANDROID PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    set_target_properties(BLACK_ANDROID PROPERTIES
        ANDROID_APK ON
        ANDROID_PACKAGE_NAME "com.black.browser"
        ANDROID_VERSION_CODE 1
        ANDROID_VERSION_NAME "1.0.0"
    )
endif()

# iOS build configuration
if(IOS)
    # iOS build is handled via Xcode project
    # The Xcode project is at ios/BLACK.xcodeproj
    message(STATUS "iOS build configured via Xcode project at ios/BLACK.xcodeproj")
endif()

qt_add_resources(BLACK_QML "qml" PREFIX "/" BASE "${CMAKE_CURRENT_SOURCE_DIR}/qml"
    FILES
        qml/MainWindow.qml
)

target_link_libraries(BLACK_QML PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickTemplates2
    Qt6::Qml
    Qt6::WebEngineQuick
    Qt6::WebChannel
    Qt6::Network
    Qt6::Positioning
)

if(WIN32)
    target_sources(BLACK_QML PRIVATE resources.rc)
    target_link_libraries(BLACK_QML PRIVATE crypt32 dwmapi)
elseif(APPLE)
    target_link_libraries(BLACK_QML PRIVATE OpenSSL::SSL OpenSSL::Crypto ${SECURITY})
    target_compile_definitions(BLACK_QML PRIVATE HAVE_OPENSSL)
elseif(LINUX)
    target_link_libraries(BLACK_QML PRIVATE OpenSSL::SSL OpenSSL::Crypto ${SECRET})
    target_compile_definitions(BLACK_QML PRIVATE HAVE_OPENSSL)
    if(SECRET)
        target_compile_definitions(BLACK_QML PRIVATE HAVE_LIBSECRET)
    endif()
endif()

if(MSVC)
    target_compile_options(BLACK_QML PRIVATE /W4)
else()
    target_compile_options(BLACK_QML PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(BLACK_QML PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_BUNDLE_NAME "BLACK"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.black.browser"
)
