# tifig

Converts HEIF images created on iOS 11 devices as fast as ~~humanly~~ possible.

*NOTE*: While we do not have the time to actively maintain tifig anymore, it is functional at a very alpha state. We are open to new maintainers taking the lead. 


## Build Dependencies

 * `libvips` >= 8.6
 * `libavcodec` >= 3.1 (ffmpeg)
 * `libswscale` >= 3.1 (ffmpeg)


#### macOS aka OSX

This one-liner should get you going:

    brew install cmake vips ffmpeg pkg-config


#### Linux

First of all, to just try out tifig, the easiest way is to [use our static builds](https://github.com/monostream/tifig/releases).

However, if you do want to build from source, verify carefully that the minimally required versions are actually shipped and installed with your distro and release.

For ffmpeg, check the output of:

    ffmpeg -version

Assuming you are using a ubuntu based system, this should help if your versions of 'libavcodec' and 'libswscale' is too old:

    sudo add-apt-repository -y ppa:jonathonf/ffmpeg-3
    sudo apt-get update
    sudo apt-get install libavcodec-dev libswscale-dev

Since tifig requires quite a modern version of `libvips`, building from source is probably required. [Follow the instructions here](http://jcupitt.github.io/libvips/install.html#building-libvips-from-a-source-tarball) .

Again on ubuntu, something like this should do the trick:

    sudo apt-get install build-essential pkg-config libglib2.0-dev libexpat1-dev libjpeg-dev libexif-dev libpng-dev libtiff-dev
    wget https://github.com/jcupitt/libvips/releases/download/v8.6.1/vips-8.6.1.tar.gz
    tar xzf vips-8.6.1.tar.gz
    cd vips-8.6.1
    ./configure
    make
    sudo make install


## Build

    git clone --recursive https://github.com/monostream/tifig.git
    mkdir tifig/build && cd tifig/build
    cmake ..
    make


## Usage

Convert the fullsize picture:

    # tifig -v -p image.heic output.jpg
    Grid is 4032x3024 pixels in tiles 8x6
    Export & decode HEVC: 97ms
    Saving image: 55ms
    Total Time: 160ms

Create a thumbnail with max width of 800px:

    # tifig -v -p --width 800 image.heic thumbnail.jpg
    Grid is 4032x3024 pixels in tiles 8x6
    Export & decode HEVC: 113ms
    Saving image: 100ms
    Total Time: 243ms


Create a cropped thumbnail to match size exactly:

    # tifig -v -p --crop --width 400 --height 400 1_portrait.heic thumbnail.jpg
    Grid is 4032x3024 pixels in tiles 8x6
    Export & decode HEVC: 105ms
    Saving image: 125ms
    Total Time: 234ms

When a size smaller or equal to 240x240 is requested, tifig will automatically use the embedded thumbnail.


## Installing

We release tifig as static x86_64 binary that should work on any linux without installing dependencies. The only requirement is glibc with a minimal version of 2.14. Just copy the binary to `/usr/local/bin` or wherever you want to.


## ToDo's

  * ~~Testing~~ 
  * ~~Create independant static binary~~
  * ~~Keep exif metadata in coverted images~~
  * Cleanup and optimizing
  * Replace Nokia library with DigiDNAs ISOBMFF parser
  * Carry over color profiles
  * Support single image HEIC
  * Improve thumbnailing
  
## Software Used / Libraries

  * HEIF by Nokia Technologies https://github.com/nokiatech/heif
  * libvips https://github.com/jcupitt/libvips
  * ffmpeg https://www.ffmpeg.org/
  * cxxopts https://github.com/jarro2783/cxxopts
  
***Suggestions for improvements and Pull Requests highly welcome!***
 



  
