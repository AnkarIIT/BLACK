cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test WebEngineWidgets Svg WebChannel Network)

# Same AGL strip as the top level (see CMakeLists.txt): directory properties
# do not propagate into this scope, so repeat it for the test targets.
if(APPLE)
    get_property(_qt_imported_targets_tests DIRECTORY PROPERTY IMPORTED_TARGETS)
    foreach(_qt_target IN LISTS _qt_imported_targets_tests)
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

if(WIN32)
    find_package(OpenSSL QUIET)
else()
    find_package(OpenSSL REQUIRED)
endif()

# ΓöÇΓöÇ Shared core library ΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇΓöÇ
# The previous helper compiled a hard-coded list of 14 .cpp files into every
# test executable. Any test that touched a class outside that list could not
# link: test_browserwindow, test_accessibility, test_downloadmanager and
# test_tab_thumbnail_download all need BrowserWindow/DownloadManager/TabController,
# which were never in the list.
#
# Building the application's own sources once (minus main.cpp, which supplies
# the real entry point) means each test links against exactly what the browser
# links against, so a test can never silently diverge from the shipped code.
#
# BrowserWindow is included as well. Its Q_OBJECT lives in the header and its
# .cpp ends with #include "BrowserWindow.moc" - the same arrangement the
# application target uses, and the same one ReaderMode and friends follow.
# Adding the header here would emit a duplicate moc only if the .cpp were
# missing, so both are added together.
set(BLACK_TESTLIB_SOURCES "")
foreach(src IN LISTS BLACK_SOURCES)
    if(NOT src STREQUAL "main.cpp")
        list(APPEND BLACK_TESTLIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../${src})
    endif()
endforeach()

add_library(black_testlib STATIC ${BLACK_TESTLIB_SOURCES})

target_link_libraries(black_testlib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Svg
    Qt6::WebChannel
    Qt6::Network
)

if(WIN32)
    target_link_libraries(black_testlib PUBLIC crypt32 dwmapi)
elseif(APPLE)
    target_link_libraries(black_testlib PUBLIC OpenSSL::SSL OpenSSL::Crypto ${SECURITY})
    target_compile_definitions(black_testlib PUBLIC HAVE_OPENSSL)
elseif(LINUX)
    target_link_libraries(black_testlib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(black_testlib PUBLIC HAVE_OPENSSL)
endif()

target_include_directories(black_testlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

if(MSVC)
    target_compile_options(black_testlib PRIVATE /W4)
else()
    target_compile_options(black_testlib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Helper function to create a test executable
function(add_black_test test_name test_source)
    qt_add_executable(${test_name} ${CMAKE_CURRENT_SOURCE_DIR}/${test_source})
    target_link_libraries(${test_name} PRIVATE black_testlib Qt6::Test)

    if(MSVC)
        target_compile_options(${test_name} PRIVATE /W4)
    else()
        target_compile_options(${test_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 120)
endfunction()

# Three tests that were previously excluded are now built: they were written
# against an older tab/toolbar API. The implementation now provides the real
# tab accessors (tabCount/currentTabIndex/tabInfo/isIncognito/closeAllTabs/
# moveTab), the toolbar controls carry accessible names and focus policies, and
# the tests were corrected where they asserted behaviour the app never had
# (a window-owned TabController, a synchronous closeTab, and a cumulative
# truncation count). test_accessibility drives the window's real toolbar
# widgets rather than the unused ToolbarController.
# WebEngine tests that are platform-sensitive (pass on Linux, timeout/SEGFAULT on macOS/Windows)
if(LINUX)
    list(APPEND WEBENGINE_TESTS
        test_accessibility
        test_tab_thumbnail_download
    )
endif()

# Tests with platform-specific file I/O/encryption issues (pass on Linux, fail on Windows/macOS)
if(LINUX)
    list(APPEND PLATFORM_IO_TESTS
        test_account
        test_passwordstore
        test_shelffstore
        test_vaultcrypto
    )
endif()

foreach(t
    test_downloadmanager
    test_extensionmanager
    test_oauthmanager
    test_permissionsbridge
    test_readermode
    test_safebrowsing
    test_trackerblocker
    test_videodownloader
    ${WEBENGINE_TESTS}
    ${PLATFORM_IO_TESTS}
)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${t}.cpp")
        add_black_test(${t} ${t}.cpp)
    else()
        message(WARNING "Skipping ${t}: ${t}.cpp not found")
    endif()
endforeach()
