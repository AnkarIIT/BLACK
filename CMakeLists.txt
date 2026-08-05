cmake_minimum_required(VERSION 3.16)
project(BLACK VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Svg WebChannel)

qt_add_executable(BLACK
    main.cpp
    BrowserWindow.cpp
    BrowserWindow.h
    SafariWebView.cpp
    SafariWebView.h
    SafariTheme.cpp
    SafariTheme.h
    TrackerBlocker.cpp
    TrackerBlocker.h
    resources.qrc
    resources.rc
)

target_link_libraries(BLACK PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Svg
    Qt6::WebChannel
)

if(MSVC)
    target_compile_options(BLACK PRIVATE /W4)
else()
    target_compile_options(BLACK PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(BLACK PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
