# tifig

Converts HEIF images created on iOS 11 devices as fast as ~~humanly~~ possible.

*This project is an early alpha and still needs a lot of tweaking!*

[![Build Status](https://travis-ci.org/monostream/tifig.svg?branch=master)](https://travis-ci.org/monostream/tifig)

## Build Dependencies

 * `libvips` >= 8.3
 * `libavcodec` >= 3.1
 * `libswscale` >= 3.1
 
Install the dependencies under a debian based distribution:

```
# sudo add-apt-repository -y ppa:teivg/graph
# sudo add-apt-repository -y ppa:jonathonf/ffmpeg-3
# suod apt-get update
# sudo apt-get install libvips-dev libavcodec-dev libswscale-dev
```

On Mac OS X:

```
# brew install vips libav
```

## Build

```
git clone https://github.com/monostream/tifig.git
cd tifig
git submodule update --init
mkdir build && cd build
cmake ..
make
```

## Usage

```
# tifig -v -p image.heic output.jpg # get the full picture
Grid is 4032x3024 pixels in tiles 8x6
Export & decode HEVC: 97ms
Saving image: 55ms
Total Time: 160ms
```

```
# tifig -v -t image.heic thumbnail.jpg # get the embedded thumbnail
Export & decode HEVC: 2ms
Saving image: 3ms
Total Time 5ms
```

## ToDo's

  * Testing 
  * Create independant static binary
  * Cleanup and optimizing 
  
## Software Used / Libraries

  * HEIF by Nokia Technologies https://github.com/nokiatech/heif
  * libvips https://github.com/jcupitt/libvips
  * libav http://www.libav.org
  * easyexif https://github.com/mayanklahiri/easyexif
  * cxxopts https://github.com/jarro2783/cxxopts
  
***Suggestions for improvements and Pull Requests highly welcome***
 



  
