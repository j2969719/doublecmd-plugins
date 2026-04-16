cmake_minimum_required(VERSION 3.16)
project(mpv_wayland LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Double Commander plugins are usually built without name prefix (lib) and with .wlx extension
set(CMAKE_SHARED_LIBRARY_PREFIX "")
if(UNIX)
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".wlx")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk
    ${MPV_INCLUDE_DIRS}
)

add_library(mpv_wayland SHARED
    src/main.cpp
    src/mpvwidget.cpp
)

target_link_libraries(mpv_wayland
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    ${MPV_LIBRARIES}
)

# extra-cmake-modules (ECM) is required per goal.md
find_package(ECM 5.80.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
