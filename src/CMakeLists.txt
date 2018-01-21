cmake_minimum_required (VERSION 2.6)
#Version Config
set (ShowPage_VERSION_MAJOR 0)
set (ShowPage_VERSION_MINOR 1)

link_directories(
    "${PROJECT_SOURCE_DIR}/src"
    )

include_directories("${PROJECT_BINARY_DIR}")

#config header file
configure_file (
    "${PROJECT_SOURCE_DIR}/src/ShowPageConf.h.in"
    "${PROJECT_BINARY_DIR}/ShowPageConf.h"
    )
add_executable(ShowPage showpage.c)
