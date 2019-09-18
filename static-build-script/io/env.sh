#!/bin/bash

export DEPS="/deps"
export TARGET="/target"

export PATH="$PATH:/target/bin"

# versions of dependencies
export VERSION_ZLIB=1.2.11
export VERSION_XML2=2.9.4
export VERSION_FFI=3.2.1
export VERSION_GETTEXT=0.19.8.1
export VERSION_GLIB=2.40.2
export VERSION_VIPS=8.6.1
export VERSION_EXIF=0.6.21
export VERSION_JPEG=1.5.1
export VERSION_LCMS2=2.8
# NOTE: libpng puts old releases in a subfolder older-releases
export VERSION_PNG16=1.6.34
export VERSION_TIFF=4.0.7
export VERSION_ORC=0.4.26
export VERSION_FFTW=3.3.6-pl1
export VERSION_NASM=2.12.02
export VERSION_FFMPEG=3.3.5

export CC="gcc-5"
export CXX="g++-5"

export PKG_CONFIG_PATH="${TARGET}/lib/pkgconfig"
export BASE_FLAGS="-s -fvisibility=hidden -I${TARGET}/include"
export CFLAGS="${BASE_FLAGS}"
export CPPFLAGS="${BASE_FLAGS} -static-libstdc++"
export CXXFLAGS="${BASE_FLAGS} -static-libstdc++"
export STATICLIB_CFLAGS="-g -O2 -fvisibility=hidden"
export LDFLAGS="-L${TARGET}/lib"
export LD_LIBRARY_PATH="${TARGET}"
