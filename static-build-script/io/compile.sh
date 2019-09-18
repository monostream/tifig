#!/bin/bash
set -e

source /io/env.sh

apt-key adv --keyserver hkp://keyserver.ubuntu.com --recv-key 084ECFC5828AB726
apt-key adv --keyserver hkp://keyserver.ubuntu.com --recv-key 1E9377A2BA9EF27F

echo "deb http://ppa.launchpad.net/george-edison55/cmake-3.x/ubuntu trusty main" | tee /etc/apt/sources.list.d/george-edison55-cmake3-trusty.list
echo "deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu trusty main" | tee /etc/apt/sources.list.d/ubuntu-toolchain-r-test-trusty.list

apt-get -y update

apt-get install -y curl xz-utils cmake3 g++-5 pkg-config autotools-dev automake libtool git gobject-introspection gtk-doc-tools

mkdir ${DEPS}
mkdir ${TARGET}

# now start building static libraries for tifig

mkdir ${DEPS}/zlib
curl -Ls https://zlib.net/zlib-${VERSION_ZLIB}.tar.xz | xz -d | tar xC ${DEPS}/zlib --strip-components=1
cd ${DEPS}/zlib
./configure --prefix=${TARGET} --static
make install

mkdir ${DEPS}/expat
curl -Ls https://github.com/libexpat/libexpat/releases/download/R_2_1_0/expat-2.1.0.tar.gz | tar xzC ${DEPS}/expat --strip-components=1
cd ${DEPS}/expat
./configure --prefix=${TARGET} --disable-shared --enable-static
make install

mkdir ${DEPS}/ffi
curl -Ls ftp://sourceware.org/pub/libffi/libffi-${VERSION_FFI}.tar.gz | tar xzC ${DEPS}/ffi --strip-components=1
cd ${DEPS}/ffi
./configure --prefix=${TARGET} --disable-shared --enable-static --disable-builddir
make install

mkdir ${DEPS}/gettext
curl -Ls http://ftp.gnu.org/pub/gnu/gettext/gettext-${VERSION_GETTEXT}.tar.xz | xz -d | tar xC ${DEPS}/gettext --strip-components=1
cd ${DEPS}/gettext
./configure --prefix=${TARGET} --disable-shared --enable-static --with-pcre=internal --disable-libmount --with-included-glib
make install

mkdir ${DEPS}/glib
curl -Ls https://ftp.acc.umu.se/pub/gnome/sources/glib/2.40/glib-${VERSION_GLIB}.tar.xz | xz -d | tar -xC ${DEPS}/glib --strip-components=1
cd ${DEPS}/glib
echo glib_cv_stack_grows=no >>glib.cache
echo glib_cv_uscore=no >>glib.cache
./configure --prefix=${TARGET} --disable-shared --enable-static --with-pcre=internal --disable-coverage --disable-dependency-tracking
make install

mkdir ${DEPS}/xml2
curl -Ls http://xmlsoft.org/sources/libxml2-${VERSION_XML2}.tar.gz | tar xzC ${DEPS}/xml2 --strip-components=1
cd ${DEPS}/xml2
./configure --prefix=${TARGET} --disable-shared --enable-static --disable-dependency-tracking \
  --without-python --without-debug --without-docbook --without-ftp --without-html --without-legacy \
  --without-pattern --without-push --without-regexps --without-schemas --without-schematron
make install

mkdir ${DEPS}/exif
curl -Ls http://netix.dl.sourceforge.net/project/libexif/libexif/${VERSION_EXIF}/libexif-${VERSION_EXIF}.tar.bz2 | tar xjC ${DEPS}/exif --strip-components=1
cd ${DEPS}/exif
autoreconf -fiv
./configure --prefix=${TARGET} --disable-shared --enable-static
make && make install

mkdir ${DEPS}/nasm
curl -Ls http://www.nasm.us/pub/nasm/releasebuilds/${VERSION_NASM}/nasm-${VERSION_NASM}.tar.xz | xz -d | tar xC ${DEPS}/nasm --strip-components=1
cd ${DEPS}/nasm
./configure --prefix=${TARGET}
make && make install

mkdir ${DEPS}/jpeg
curl -Ls https://github.com/libjpeg-turbo/libjpeg-turbo/archive/${VERSION_JPEG}.tar.gz | tar xzC ${DEPS}/jpeg --strip-components=1
cd ${DEPS}/jpeg
autoreconf -fiv
./configure --prefix=${TARGET} --disable-shared --enable-static --with-jpeg8 --with-turbojpeg
make install

mkdir ${DEPS}/lcms2
curl -Ls http://netix.dl.sourceforge.net/project/lcms/lcms/${VERSION_LCMS2}/lcms2-${VERSION_LCMS2}.tar.gz | tar xzC ${DEPS}/lcms2 --strip-components=1
cd ${DEPS}/lcms2
./configure --prefix=${TARGET} --disable-shared --enable-static
make install

mkdir ${DEPS}/png
curl -Ls http://netix.dl.sourceforge.net/project/libpng/libpng16/${VERSION_PNG16}/libpng-${VERSION_PNG16}.tar.xz | xz -d | tar xC ${DEPS}/png --strip-components=1
cd ${DEPS}/png
./configure --prefix=${TARGET} --disable-shared --enable-static
make install

mkdir ${DEPS}/tiff
curl -Ls http://download.osgeo.org/libtiff/tiff-${VERSION_TIFF}.tar.gz | tar xzC ${DEPS}/tiff --strip-components=1
cd ${DEPS}/tiff
./configure --prefix=${TARGET} --disable-shared --enable-static --disable-mdi --disable-pixarlog --disable-cxx
make install

mkdir ${DEPS}/orc
curl -Ls http://gstreamer.freedesktop.org/data/src/orc/orc-${VERSION_ORC}.tar.xz | xz -d | tar xC ${DEPS}/orc --strip-components=1
cd ${DEPS}/orc
./configure --prefix=${TARGET} --disable-shared --enable-static
make install

mkdir ${DEPS}/fftw
curl -Ls http://www.fftw.org/fftw-${VERSION_FFTW}.tar.gz | tar xzC ${DEPS}/fftw --strip-components=1
cd ${DEPS}/fftw
./configure --prefix=${TARGET} --disable-shared --enable-static
make install

mkdir ${DEPS}/vips
curl -Ls https://github.com/jcupitt/libvips/archive/v${VERSION_VIPS}/vips-${VERSION_VIPS}.tar.gz | tar xzC ${DEPS}/vips --strip-components=1
cd ${DEPS}/vips
./autogen.sh --prefix=${TARGET} --disable-shared --enable-static --without-radiance --without-analyze
make install

mkdir ${DEPS}/ffmpeg
curl -Ls https://github.com/FFmpeg/FFmpeg/archive/n${VERSION_FFMPEG}.tar.gz | tar xzC ${DEPS}/ffmpeg --strip-components=1
cd ${DEPS}/ffmpeg
./configure --prefix=${TARGET} --disable-shared --enable-static --disable-encoders --disable-decoders --enable-decoder=hevc --disable-parsers --enable-parser=hevc \
  --disable-programs --disable-doc --disable-avdevice --disable-avformat --disable-avfilter --disable-indevs --disable-outdevs --disable-cuvid --disable-bsfs
make install

git clone https://github.com/monostream/tifig.git ${DEPS}/tifig
cd ${DEPS}/tifig
VERSION_TIFIG=$(git describe --tags `git rev-list --tags --max-count=1`)
git checkout $VERSION_TIFIG
git submodule update --init
mkdir build && cd build
cmake -DSTATIC_BUILD=ON ..
make

output=/io/tifig-binary
mkdir -p ${output}
echo ${VERSION_TIFIG} >> ${output}/version.txt
cp tifig ${output}

GZIP=-9 tar czf /io/tifig-static-${VERSION_TIFIG}.tar.gz ${output}/*
