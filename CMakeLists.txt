cmake_minimum_required(VERSION 3.16)
project(BLACK VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Svg WebChannel Network Test)

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
)

if(WIN32)
    target_sources(BLACK PRIVATE resources.rc)
    target_link_libraries(BLACK PRIVATE crypt32)
endif()

qt_add_resources(BLACK "resources" PREFIX "/" BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES
        onboarding_experience.html
        vendor/three.min.js
        vendor/gsap.min.js
        startpage_enhanced.html
        bookmarks.html
        history.html
        settings.html
        extensions.html
        features.html
        privacyreport.html
        safebrowsing_warning.html
        chrome-theme.css
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

enable_testing()

add_executable(test_vaultcrypto tests/test_vaultcrypto.cpp VaultCrypto.cpp OSPaths.cpp)
target_link_libraries(test_vaultcrypto PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME test_vaultcrypto COMMAND test_vaultcrypto)
set_tests_properties(test_vaultcrypto PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_trackerblocker tests/test_trackerblocker.cpp TrackerBlocker.cpp SafeBrowsing.cpp OSPaths.cpp)
target_link_libraries(test_trackerblocker PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::WebEngineWidgets Qt6::Test)
add_test(NAME test_trackerblocker COMMAND test_trackerblocker)
set_tests_properties(test_trackerblocker PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_passwordstore tests/test_passwordstore.cpp PasswordStore.cpp VaultCrypto.cpp OSPaths.cpp)
target_link_libraries(test_passwordstore PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME test_passwordstore COMMAND test_passwordstore)
set_tests_properties(test_passwordstore PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_oauthmanager tests/test_oauthmanager.cpp OAuthManager.cpp VaultCrypto.cpp OSPaths.cpp)
target_link_libraries(test_oauthmanager PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Test)
add_test(NAME test_oauthmanager COMMAND test_oauthmanager)
set_tests_properties(test_oauthmanager PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
