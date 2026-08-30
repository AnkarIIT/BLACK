cmake_minimum_required(VERSION 3.16)
project(BLACK_Tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Test WebEngineWidgets Svg WebChannel Network)
find_package(OpenSSL REQUIRED)

# Test executable
qt_add_executable(BLACK_Tests
    test_vaultcrypto.cpp
    test_trackerblocker.cpp
    test_passwordstore.cpp
    test_oauthmanager.cpp
    test_shelffstore.cpp
    test_permissionsbridge.cpp
    test_main.cpp
)

target_link_libraries(BLACK_Tests PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::WebEngineWidgets
    Qt6::Svg
    Qt6::WebChannel
    Qt6::Network
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CMAKE_SOURCE_DIR}/../VaultCrypto.cpp
    ${CMAKE_SOURCE_DIR}/../TrackerBlocker.cpp
    ${CMAKE_SOURCE_DIR}/../PasswordStore.cpp
    ${CMAKE_SOURCE_DIR}/../OAuthManager.cpp
    ${CMAKE_SOURCE_DIR}/../ShelfStore.cpp
    ${CMAKE_SOURCE_DIR}/../PermissionsBridge.cpp
    ${CMAKE_SOURCE_DIR}/../OSPaths.cpp
    ${CMAKE_SOURCE_DIR}/../SafariTheme.cpp
    ${CMAKE_SOURCE_DIR}/../BrowserSettings.cpp
    ${CMAKE_SOURCE_DIR}/../SafeBrowsing.cpp
    ${CMAKE_SOURCE_DIR}/../AppearanceManager.cpp
    ${CMAKE_SOURCE_DIR}/../Account.cpp
    ${CMAKE_SOURCE_DIR}/../ExtensionManager.cpp
    ${CMAKE_SOURCE_DIR}/../BookmarkImporter.cpp
)

target_include_directories(BLACK_Tests PRIVATE ${CMAKE_SOURCE_DIR}/..)

if(MSVC)
    target_compile_options(BLACK_Tests PRIVATE /W4)
else()
    target_compile_options(BLACK_Tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(BLACK_Tests PRIVATE HAVE_OPENSSL)