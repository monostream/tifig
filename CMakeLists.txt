cmake_minimum_required(VERSION 3.5)

set(PACKAGE "tifig")
project(${PACKAGE})

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})

find_package(PkgConfig)
pkg_check_modules(AVCODEC_PKG libavcodec REQUIRED)
pkg_check_modules(AVUTIL_PKG libavutil REQUIRED)
pkg_check_modules(SWSCALE_PKG libswscale REQUIRED)
pkg_check_modules(VIPS_PKG vips-cpp REQUIRED)

find_library(AVCODEC_LIBRARY
        NAMES ${AVCODEC_PKG_LIBRARIES}
        PATHS ${AVCODEC_PKG_LIBRARY_DIRS})

find_library(AVUTIL_LIBRARY
        NAMES ${AVUTIL_PKG_LIBRARIES}
        PATHS ${AVUTIL_PKG_LIBRARY_DIRS})

find_library(SWSCALE_LIBRARY
        NAMES ${SWSCALE_PKG_LIBRARIES}
        PATHS ${SWSCALE_PKG_LIBRARY_DIRS})

find_library(VIPS_LIBRARY
        NAMES ${VIPS_PKG_LIBRARIES}
        PATHS ${VIPS_PKG_LIBRARY_DIRS})


#set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-implicit-fallthrough")
set(CMAKE_CXX_FLAGS_RELEASE "-Wno-implicit-fallthrough")

set(LIBHEIF_DIR ${PROJECT_SOURCE_DIR}/lib/heif)
set(SOURCE_FILES
        src/main.cpp
        lib/easyexif/exif.cpp)
set(TIFIG tifig)

add_subdirectory(${LIBHEIF_DIR})
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/cxxopts)

add_executable(${TIFIG} ${SOURCE_FILES})

include_directories(${LIBHEIF_DIR}/Srcs/common)
include_directories(${LIBHEIF_DIR}/Srcs/reader)
include_directories(lib/cxxopts/include)
include_directories(lib/easyexif)
include_directories(${VIPS_PKG_INCLUDE_DIRS})

target_link_libraries(${TIFIG} heifreader cxxopts)
target_link_libraries(${TIFIG} ${VIPS_PKG_LDFLAGS} ${AVCODEC_PKG_LDFLAGS} ${AVUTIL_PKG_LDFLAGS} ${SWSCALE_PKG_LDFLAGS} pthread)