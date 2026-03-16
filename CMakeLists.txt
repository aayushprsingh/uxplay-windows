cmake_minimum_required(VERSION 3.20)
project(MyAirPlay)

set(CMAKE_CXX_STANDARD 17)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

# GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED
    gstreamer-1.0
    gstreamer-video-1.0
    gstreamer-app-1.0
)

# UxPlay come subdirectory (compila uxplay come libreria, non executable)
# Disabilita il target eseguibile di uxplay
set(BUILD_UXPLAY_EXECUTABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/uxplay)

# Il tuo eseguibile
qt_add_executable(MyAirPlay
    main.cpp
    airplaywindow.cpp
    airplaywindow.h
)

target_include_directories(MyAirPlay PRIVATE
    ${GST_INCLUDE_DIRS}
    lib/uxplay         # per #include <uxplay/uxplay.h>
)

target_link_libraries(MyAirPlay PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    uxplay             # target CMake definito in lib/uxplay
    ${GST_LIBRARIES}
)
