cmake_minimum_required(VERSION 3.16)
project(BLACK_Tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Test WebEngineWidgets Svg WebChannel Network)

# OpenSSL is only required on non-Windows for AES-GCM in VaultCrypto
# On Windows, DPAPI is used which doesn't require OpenSSL
if(WIN32)
    find_package(OpenSSL QUIET)
else()
    find_package(OpenSSL REQUIRED)
endif()

# Helper function to create a test executable
function(add_black_test test_name test_source)
    qt_add_executable(${test_name}
        ${test_source}
        ${CMAKE_CURRENT_SOURCE_DIR}/../VaultCrypto.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../TrackerBlocker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../PasswordStore.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../OAuthManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../ShelfStore.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../PermissionsBridge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../OSPaths.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../SafariTheme.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../BrowserSettings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../SafeBrowsing.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../AppearanceManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../Account.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../ExtensionManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../BookmarkImporter.cpp
    )

    target_link_libraries(${test_name} PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::WebEngineWidgets
        Qt6::Svg
        Qt6::WebChannel
        Qt6::Network
    )

    if(WIN32)
        target_link_libraries(${test_name} PRIVATE crypt32)
    else()
        target_link_libraries(${test_name} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(${test_name} PRIVATE HAVE_OPENSSL)
    endif()

    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# Create individual test executables
add_black_test(test_vaultcrypto test_vaultcrypto.cpp)
add_black_test(test_trackerblocker test_trackerblocker.cpp)
add_black_test(test_passwordstore test_passwordstore.cpp)
add_black_test(test_oauthmanager test_oauthmanager.cpp)
add_black_test(test_shelffstore test_shelffstore.cpp)
add_black_test(test_permissionsbridge test_permissionsbridge.cpp)
add_black_test(test_safebrowsing test_safebrowsing.cpp)
add_black_test(test_tab_thumbnail_download test_tab_thumbnail_download.cpp)
add_black_test(test_browserwindow test_browserwindow.cpp)
add_black_test(test_extensionmanager test_extensionmanager.cpp)
add_black_test(test_account test_account.cpp)
add_black_test(test_downloadmanager test_downloadmanager.cpp)
add_black_test(test_readermode test_readermode.cpp)
add_black_test(test_accessibility test_accessibility.cpp)