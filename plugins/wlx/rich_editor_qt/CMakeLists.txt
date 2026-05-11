cmake_minimum_required(VERSION 3.16)
project(rich_editor_qt VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6TextEditor REQUIRED)
find_package(KF6Config REQUIRED)

add_library(rich_editor_qt SHARED
    src/wlx_plugin.cpp
    src/editor_widget.cpp
)

target_compile_definitions(rich_editor_qt PRIVATE
    RICH_EDITOR_QT_LOG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/logs"
)

target_link_libraries(rich_editor_qt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::TextEditor
    KF6::ConfigCore
)

set_target_properties(rich_editor_qt PROPERTIES
    PREFIX ""
    SUFFIX ".wlx"
)
