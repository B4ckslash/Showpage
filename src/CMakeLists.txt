cmake_minimum_required (VERSION 3.4)
#Version Config
set (ShowPage_VERSION_MAJOR 0)
set (ShowPage_VERSION_MINOR 1)

link_directories(
    "${PROJECT_SOURCE_DIR}/src"
    )

include_directories("${PROJECT_BINARY_DIR}/lib")

find_package(LibXml2)

#config header file
configure_file (
    "${PROJECT_SOURCE_DIR}/src/ShowPageConf.h.in"
    "${PROJECT_BINARY_DIR}/lib/ShowPageConf.h"
    )
add_executable(ShowPage showpage.c)
target_link_libraries(ShowPage ${LIBXML2_LIBRARIES})
