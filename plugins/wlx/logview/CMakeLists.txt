cmake_minimum_required(VERSION 3.16)
project(wlx-log-viewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Required for Q_OBJECT macro — generates moc files for vtable/signal-slot metadata
set(CMAKE_AUTOMOC ON)

# Visibility hidden by default as requested in spec
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(re2 REQUIRED)

# Need Qt6::WaylandClient? If not just Widgets and Core
# For inotify, it's native linux.

# Build the shared library (WLX plugin)
add_library(logviewer_wlx SHARED
    src/wlx_plugin.cpp
    src/LogViewerWidget.cpp
    src/LogModel.cpp
)

set_target_properties(logviewer_wlx PROPERTIES
    PREFIX ""
    SUFFIX ".wlx"
)

target_link_libraries(logviewer_wlx PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    re2::re2
)

# Only export specific functions
# This is usually handled by `extern "C" __attribute__((visibility("default")))` in the code,
# combined with the hidden preset.
