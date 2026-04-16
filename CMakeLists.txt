cmake_minimum_required(VERSION 3.16)
project(BrowserClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components including WebEngineWidgets
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets Svg)
qt_standard_project_setup()

# Set application executable
add_executable(BrowserClone
    main.cpp
    BrowserWindow.cpp
    BrowserWindow.h
)

# Link against Qt libraries
target_link_libraries(BrowserClone PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Svg
)

# Deployment requirements (Windows/macOS specific GUI handles)
set_target_properties(BrowserClone PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
