cmake_minimum_required(VERSION 3.16)
project(kpart_host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Double Commander plugin settings
set(CMAKE_SHARED_LIBRARY_PREFIX "")
if(UNIX)
    set(CMAKE_SHARED_LIBRARY_SUFFIX ".wlx")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(KF6Parts REQUIRED)
find_package(KF6KIO REQUIRED)
find_package(KF6CoreAddons REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk
)

add_library(kpart_host SHARED
    src/main.cpp
    src/kpartwidget.cpp
)

target_link_libraries(kpart_host
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    KF6::Parts
    KF6::KIOFileWidgets
    KF6::CoreAddons
)

# Use ECM for standard installation paths and KDE integration
find_package(ECM 5.80.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
