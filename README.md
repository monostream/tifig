# tifig

Converts HEIF images created on iOS 11 devices to JPEG as fast as possible.

*This project is an early alpha and still needs a lot of tweaking!*

## Dependencies

 * `ffmpeg` >= 3.3
 * `libvips` >= 8.3

## Usage

### Build

```
git clone https://github.com/monostream/tifig.git
cd tifig
git submodule update --init
mkdir build && cd build
cmake ..
make
```

### Usage

```
tifig -v -q 75 image.heic output.jpg 
```

## ToDo's

  * Testing 
  * Create independant static binary 
  * Improve conversation by using libAV directly 
  * Cleanup and optimizing 
  
## Software Used / Libraries

  * HEIF by Nokia Technologies https://github.com/nokiatech/heif
  * libvips https://github.com/jcupitt/libvips
  * FFmpeg https://github.com/FFmpeg/FFmpeg
  * easyexif https://github.com/mayanklahiri/easyexif
  * cxxopts https://github.com/jarro2783/cxxopts
  
***Suggestions for improvements and Pull Requests highly welcome***
 



  
