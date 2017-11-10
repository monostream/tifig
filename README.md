# tifig

Converts HEIF images created on iOS 11 devices to JPEG as fast as possible.

*This project is an early alpha and still needs a lot of tweaking!*

## Build Dependencies

 * `libav` >= 12.2
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
# tifig -v image.heic output.jpg # get the full picture

```

```
# tifig -v -t image.heic thumbnail.jpg # get the embedded thumbnail

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
 



  
