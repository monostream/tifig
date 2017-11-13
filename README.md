# tifig

Converts HEIF images created on iOS 11 devices to JPEG as fast as possible.

*This project is an early alpha and still needs a lot of tweaking!*

## Build Dependencies

 * `libav` >= 12.2
 * `libvips` >= 8.3
 
Install the dependencies under a debian based distribution:

```
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
# tifig -v image.heic output.jpg # get the full picture
Grid is 4032x3024 pixels in tiles 8x6
Export & decode tiles 190ms
Building image 125ms
Total Time 324ms
```

```
# tifig -v -t image.heic thumbnail.jpg # get the embedded thumbnail
320x240 pixels 
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
 



  
