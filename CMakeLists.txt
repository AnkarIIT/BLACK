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
if(ANDROID)
    message(FATAL_ERROR
        "BLACK cannot be built for Android as-is: Qt WebEngine (Widgets) used by "
        "this project is not supported on Android.")
elseif(IOS)
    message(FATAL_ERROR
        "BLACK cannot be built for iOS: Qt WebEngine is not available on iOS.")
endif()

# Windows on ARM support
if(WIN32 AND BLACK_ARM64)
    message(STATUS "Windows on ARM (ARM64) detected - enabling ARM64 optimizations")
    add_compile_options(/favor:ARM64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DARM64")
endif()

# Only the components actually used are required. Positioning/Quick were
# previously REQUIRED but never linked or used, which only made configure
# harder on machines without those modules installed. Test is needed by the
# fuzz harnesses (they reuse the QtTest macros).
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Svg WebChannel Network Test)

# Qt 6.8's WebEngine imported targets propagate `-framework AGL`, which Apple
# removed from the macOS 15 SDK, so every link fails with
# `ld: framework 'AGL' not found`. None of our objects reference AGL symbols;
# strip the flag from every Qt6 imported target's link interface. (If a future
# Qt actually needs AGL symbols, the link will fail with undefined symbols
# instead, which is the observable signal to revisit this.)
if(APPLE)
    get_property(_qt_imported_targets DIRECTORY PROPERTY IMPORTED_TARGETS)
    foreach(_qt_target IN LISTS _qt_imported_targets)
        foreach(_qt_prop INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
            get_target_property(_qt_vals ${_qt_target} ${_qt_prop})
            if(_qt_vals AND NOT _qt_vals MATCHES "NOTFOUND$")
                # WrapOpenGL carries the framework as a full SDK-absolute path,
                # which CMake renders as `-framework AGL` at link time.
                list(REMOVE_ITEM _qt_vals "-framework AGL" "/System/Library/Frameworks/AGL.framework")
                set_target_properties(${_qt_target} PROPERTIES ${_qt_prop} "${_qt_vals}")
            endif()
        endforeach()
    endforeach()
endif()

# OpenSSL is only required on non-Windows for AES-GCM in VaultCrypto
# On Windows, DPAPI is used which doesn't require OpenSSL

# macOS: Homebrew OpenSSL is in non-standard location; help CMake find it
if(APPLE)
    # Homebrew on Apple Silicon: /opt/homebrew/opt/openssl@3 or /opt/homebrew/opt/openssl
    # Homebrew on Intel: /usr/local/opt/openssl@3 or /usr/local/opt/openssl
    set(OPENSSL_ROOT_DIR_HINTS
        "/opt/homebrew/opt/openssl@3"
        "/opt/homebrew/opt/openssl"
        "/usr/local/opt/openssl@3"
        "/usr/local/opt/openssl"
    )
    find_package(OpenSSL REQUIRED HINTS ${OPENSSL_ROOT_DIR_HINTS})
else()
    if(WIN32)
        find_package(OpenSSL QUIET)
    else()
        find_package(OpenSSL REQUIRED)
    endif()
endif()

# macOS Keychain (Security framework) for VaultCrypto master key storage
if(APPLE)
    find_library(SECURITY Security)
    if(NOT SECURITY)
        message(FATAL_ERROR "Security framework not found on macOS")
    endif()
endif()

# Linux libsecret for VaultCrypto master key storage. The headers live under
# /usr/include/libsecret-1 (not on the default include path).
if(LINUX)
    find_path(LIBSECRET_INCLUDE_DIR
        NAMES secret/secret.h
        PATHS /usr/include/libsecret-1 /usr/local/include/libsecret-1
    )
    find_library(LIBSECRET_LIBRARY
        NAMES secret-1
        PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
    )
    if(NOT LIBSECRET_INCLUDE_DIR OR NOT LIBSECRET_LIBRARY)
        message(WARNING "libsecret not found on Linux, falling back to file-based master key storage")
    endif()
endif()

# ── Sources ──────────────────────────────────────────────────────────────────
# Every file listed here must exist. A previous revision referenced ~70 files
# that were never written (ProfileManager, Sandbox, WebAuthn, Sync*, ReaderMode,
# TabGroup, ...), which made the build fail at configure time. Classes that are
# placeholders now ship as explicitly-documented stubs.
set(BLACK_SOURCES
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
    ExtensionApi.cpp
    ExtensionApi.h
    Account.cpp
    Account.h
    SafeBrowsing.cpp
    SafeBrowsing.h
    OSPaths.cpp
    OSPaths.h
    OnboardingBridge.cpp
    OnboardingBridge.h
    AppearanceManager.cpp
    AppearanceManager.h
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
    TabManager.cpp
    TabManager.h
    ToolbarController.cpp
    ToolbarController.h
    TabController.cpp
    TabController.h
    CrashReporter.cpp
    CrashReporter.h
    CrashReporting.h
    FilterListParser.cpp
    FingerprintProtection.cpp
    FingerprintProtection.h
    ReaderMode.cpp
    ReaderMode.h
    Sandbox.cpp
    Sandbox.h
    CookiePartition.cpp
    CookiePartition.h
    SyncManager.cpp
    SyncManager.h
    TabGroupManager.cpp
    TabGroupManager.h
    TranslationManager.cpp
    TranslationManager.h
    MacOSWindowHelper.cpp
    MacOSWindowHelper.h
    ProfileManager.cpp
    ProfileManager.h
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
    WebAuthn.cpp
    WebAuthn.h
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
    AdvancedSecurity.cpp
    AdvancedSecurity.h
    VideoDownloader.cpp
    VideoDownloader.h
)

qt_add_executable(BLACK ${BLACK_SOURCES})

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

target_include_directories(BLACK PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
    target_sources(BLACK PRIVATE resources.rc)
    target_link_libraries(BLACK PRIVATE crypt32 dwmapi)
elseif(APPLE)
    target_link_libraries(BLACK PRIVATE OpenSSL::SSL OpenSSL::Crypto ${SECURITY})
    target_compile_definitions(BLACK PRIVATE HAVE_OPENSSL)
elseif(LINUX)
    target_link_libraries(BLACK PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    if(LIBSECRET_INCLUDE_DIR AND LIBSECRET_LIBRARY)
        target_include_directories(BLACK PRIVATE ${LIBSECRET_INCLUDE_DIR})
        target_link_libraries(BLACK PRIVATE ${LIBSECRET_LIBRARY})
        target_compile_definitions(BLACK PRIVATE HAVE_LIBSECRET)
    endif()
    target_compile_definitions(BLACK PRIVATE HAVE_OPENSSL)
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

if(ENABLE_TSAN AND NOT MSVC)
    target_compile_options(BLACK PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
    target_link_options(BLACK PRIVATE -fsanitize=thread)
endif()

if(ENABLE_MSAN AND NOT MSVC)
    target_compile_options(BLACK PRIVATE -fsanitize=memory -fno-omit-frame-pointer)
    target_link_options(BLACK PRIVATE -fsanitize=memory)
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

# ── Fuzzing ──────────────────────────────────────────────────────────────────
# Harnesses are linked against black_testlib (the application's own sources,
# minus main.cpp) so they exercise the real code paths.
#
# The previous revision compiled ${BLACK_SOURCES} directly into each harness and
# linked only Qt6::Core. That could never link: the source list contains
# main.cpp (a second main(), clashing with libFuzzer's own) and the browser
# sources need Qt6::Widgets/WebEngineWidgets, which were not linked. "libFuzzer"
# was also listed as a link *library* - it is a compiler/linker driver
# (-fsanitize=fuzzer), not a target.
if(ENABLE_FUZZING AND NOT MSVC)
    if(TARGET black_testlib)
        foreach(harness
            fuzzer_safebrowsing
            fuzzer_trackerblocker
            fuzzer_vaultcrypto
            fuzzer_urlparser
        )
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${harness}.cpp")
                add_executable(${harness} tests/${harness}.cpp)
                target_link_libraries(${harness} PRIVATE black_testlib Qt6::Test)
                target_include_directories(${harness} PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR}
                )
                target_compile_options(${harness} PRIVATE -fsanitize=fuzzer-no-link -fno-omit-frame-pointer)
                target_link_options(${harness} PRIVATE -fsanitize=fuzzer,address)
            endif()
        endforeach()
    else()
        message(WARNING "ENABLE_FUZZING=ON but black_testlib is unavailable - skipping fuzz targets")
    endif()
endif()