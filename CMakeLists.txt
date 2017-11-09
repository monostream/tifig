cmake_minimum_required(VERSION 3.5)

project(tifig)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig)
pkg_search_module(VIPS REQUIRED vips-cpp)
find_package(LibAV REQUIRED)

#set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-implicit-fallthrough")

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
include_directories(${VIPS_INCLUDE_DIRS})

target_link_libraries(${TIFIG} heifreader cxxopts)
target_link_libraries(${TIFIG} ${VIPS_CFLAGS} ${VIPS_LDFLAGS} ${LIBAV_LIBRARIES})