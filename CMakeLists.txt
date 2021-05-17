cmake_minimum_required(VERSION 3.19)
project(opengl_qt)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(opengl_qt
        gpc.cpp
        gpc.h
        main.cpp
        mainwindow.cpp
        mainwindow.h
        openglwidget.cpp
        openglwidget.h
        openglwindow.cpp
        openglwindow.h
        renderlayer.cpp
        renderlayer.h
        rtreeindex.cpp
        rtreeindex.h
        utils.cpp
        utils.h)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

FIND_PATH(GDAL_INCLUDE_DIR gdal.h "$ENV{LIB_DIR}/include")
FIND_LIBRARY(GDAL_LIBRARY NAMES gdal PATHS "$ENV{LIB_DIR}/lib")

target_link_libraries(opengl_qt
        PUBLIC
        Qt5::Widgets
        ${GDAL_LIBRARY}
        )

