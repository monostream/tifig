# tifig

Converts HEIF images created on iOS 11 devices as fast as ~~humanly~~ possible.

*This project is an early alpha and still needs a lot of tweaking!*

[![Build Status](https://travis-ci.org/monostream/tifig.svg?branch=master)](https://travis-ci.org/monostream/tifig)

## Build Dependencies

 * `libvips` >= 8.5
 * `libavcodec` >= 3.1
 * `libswscale` >= 3.1
 * `libexif` >= 0.6.14
 
Install the dependencies under a ubuntu based distribution (Should work for xenial, zesty and artful):

```
# sudo add-apt-repository -y ppa:dhor/myway
# sudo add-apt-repository -y ppa:jonathonf/ffmpeg-3
# suod apt-get update
# sudo apt-get install libvips-dev libavcodec-dev libswscale-dev
```

On Mac OS X:

```
# brew install vips ffmpeg
```

## Build

```
git clone --recursive https://github.com/monostream/tifig.git
mkdir tifig/build && cd tifig/build
cmake ..
make
```

## Usage

Convert the fullsize picture:
```
# tifig -v -p image.heic output.jpg
Grid is 4032x3024 pixels in tiles 8x6
Export & decode HEVC: 97ms
Saving image: 55ms
Total Time: 160ms
```

Create a thumbnail with max width of 800px:
```
# tifig -v -p --width 800 image.heic thumbnail.jpg 
Grid is 4032x3024 pixels in tiles 8x6
Export & decode HEVC: 113ms
Saving image: 100ms
Total Time: 243ms
```

Create a cropped thumbnail to match size exactly:
```
# tifig -v -p --crop --width 400 --height 400 1_portrait.heic thumbnail.jpg
Grid is 4032x3024 pixels in tiles 8x6
Export & decode HEVC: 105ms
Saving image: 125ms
Total Time: 234ms
```
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
 



  
