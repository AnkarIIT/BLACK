cmake_minimum_required(VERSION 3.16)
project(SafariClone VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Svg)

qt_add_executable(SafariClone
    main.cpp
    BrowserWindow.cpp
    BrowserWindow.h
    SafariWebView.cpp
    SafariWebView.h
    SafariTheme.cpp
    SafariTheme.h
    resources.qrc
)

target_link_libraries(SafariClone PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Svg
)

if(MSVC)
    target_compile_options(SafariClone PRIVATE /W4)
else()
    target_compile_options(SafariClone PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(SafariClone PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
