cmake_minimum_required(VERSION 3.5)

set(PACKAGE "tifig")
project(${PACKAGE})

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wno-implicit-fallthrough")

option(STATIC_BUILD "Build tifig static")

if (STATIC_BUILD)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -static-libstdc++")	
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_STATIC_LIBS ON)
endif()

find_package(PkgConfig)

pkg_check_modules(AVCODEC libavcodec REQUIRED)
pkg_check_modules(AVUTIL libavutil REQUIRED)
pkg_check_modules(SWSCALE libswscale REQUIRED)
pkg_check_modules(VIPS vips-cpp REQUIRED)


set(TIFIG_INCLUDE_DIRS "${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS} ${SWSCALE_INCLUDE_DIRS} ${VIPS_INCLUDE_DIRS}")

if (STATIC_BUILD)    
    set(TIFIG_PKG_LIBS "${AVCODEC_STATIC_LDFLAGS} ${AVUTIL_STATIC_LDFLAGS} ${SWSCALE_STATIC_LDFLAGS} ${VIPS_STATIC_LDFLAGS}") 
else()
    set(TIFIG_PKG_LIBS "${AVCODEC_LDFLAGS} ${AVUTIL_LDFLAGS} ${SWSCALE_LDFLAGS} ${VIPS_LDFLAGS}")    
endif()


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

include_directories(${TIFIG_INCLUDE_DIRS})

target_link_libraries(${TIFIG} heifreader cxxopts pthread)
target_link_libraries(${TIFIG} ${TIFIG_PKG_LIBS})
