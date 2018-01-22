cmake_minimum_required(VERSION 3.5)

set(PACKAGE "tifig")
project(${PACKAGE})

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Needed for heif library from nokia
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-unused-parameter -Wno-unused-variable")

option(STATIC_BUILD "Build tifig static")

if (STATIC_BUILD)
    # Strip static build
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif()

find_package(PkgConfig)

pkg_check_modules(AVCODEC libavcodec REQUIRED)
pkg_check_modules(AVUTIL libavutil REQUIRED)
pkg_check_modules(SWSCALE libswscale REQUIRED)
pkg_check_modules(VIPS vips-cpp REQUIRED)

if (STATIC_BUILD)
    pkg_check_modules(EXPAT expat REQUIRED)
    set(TIFIG_LDFLAGS
            ${AVCODEC_STATIC_LDFLAGS}
            ${AVUTIL_STATIC_LDFLAGS}
            ${SWSCALE_STATIC_LDFLAGS}
            ${VIPS_STATIC_LDFLAGS}
            ${EXPAT_STATIC_LDFLAGS})
else()
    set(TIFIG_LDFLAGS
            ${AVCODEC_LDFLAGS}
            ${AVUTIL_LDFLAGS}
            ${SWSCALE_LDFLAGS}
            ${VIPS_LDFLAGS})
endif()

set(LIBHEIF_DIR ${PROJECT_SOURCE_DIR}/lib/heif)
set(SOURCE_FILES
        src/heif.hpp
        src/hevc_decode.hpp
        src/imaging.hpp
        src/loader.hpp
        src/types.hpp
        src/main.cpp)
set(TIFIG tifig)

add_subdirectory(${LIBHEIF_DIR})
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/cxxopts)

add_executable(${TIFIG} ${SOURCE_FILES})

include_directories(${LIBHEIF_DIR}/Srcs/common)
include_directories(${LIBHEIF_DIR}/Srcs/reader)
include_directories(lib/cxxopts/include)

include_directories(${AVCODEC_INCLUDE_DIRS})
include_directories(${AVUTIL_INCLUDE_DIRS})
include_directories(${SWSCALE_INCLUDE_DIRS})
include_directories(${VIPS_INCLUDE_DIRS})

target_link_libraries(${TIFIG} heifreader cxxopts pthread)
target_link_libraries(${TIFIG} ${TIFIG_LDFLAGS})
