#ifndef TIFIG_IMAGING
#define TIFIG_IMAGING

#include <vips/vips8>
#include <chrono>

#include "types.hpp"

using namespace std;
using namespace vips;

/**
 * Add Exif metadata to image
 * @param exifData
 * @return
 */
void addExifMetadata(DataVector &exifData, VImage &image)
{
    uint8_t* exifDataPtr = &exifData[0];
    uint32_t exifDataLength = static_cast<uint32_t>(exifData.size());

    image.set(VIPS_META_EXIF_NAME, nullptr, exifDataPtr, exifDataLength);
}

/**
 * Resize the output image using vips thumbnail logic
 * @param img
 * @param options
 * @return
 */
VImage createVipsThumbnail(VImage& img, Opts& options)
{
    // This is a bit strange, we have to encode the image into a buffer first
    // However, TIFF encoding is quite fast
    VipsBlob* imgBlob = img.autorot().tiffsave_buffer(VImage::option()->set("strip", true));

    // Build thumbnail options from aguments
    VOption* thumbnailOptions = VImage::option();
    if (options.height > 0)
        thumbnailOptions->set("height", options.height);
    if (options.crop)
        thumbnailOptions->set("crop", VIPS_INTERESTING_CENTRE);

    // Now load vips thumbnail from that buffer
    return VImage::thumbnail_buffer(imgBlob, options.width, thumbnailOptions);
}

/**
 * Save created image to file
 * @param img
 * @param fileName
 * @param options
 */
void saveOutputImageToFile(VImage &img, Opts &options)
{
    if (options.outputPath.empty()) {
        throw new logic_error("Can't save to file without 'outputPath' option");
    }

    chrono::steady_clock::time_point begin_buildImage = chrono::steady_clock::now();

    char * outName = const_cast<char *>(options.outputPath.c_str());

    string ext = options.outputPath.substr(options.outputPath.find_last_of('.') + 1);

    // Supported image output formats
    set<string> jpgExt = {"jpg", "jpeg", "JPG", "JPEG"};
    set<string> pngExt = {"png", "PNG"};
    set<string> tiffExt = {"tiff", "TIFF"};
    set<string> ppmExt = {"ppm", "PPM"};

    if (jpgExt.find(ext) != jpgExt.end()) {
        img.jpegsave(outName, VImage::option()->set("Q", options.quality));
    } else if (tiffExt.find(ext) != tiffExt.end()) {
        img.autorot().tiffsave(outName, VImage::option()->set("strip", true));
    } else if (pngExt.find(ext) != pngExt.end()) {
        img.autorot().pngsave(outName);
    } else if (ppmExt.find(ext) != ppmExt.end()) {
        img.autorot().ppmsave(outName);
    } else {
        throw logic_error("Unknown image extension: " + ext);
    }

    chrono::steady_clock::time_point end_buildImage = chrono::steady_clock::now();
    long buildImageTime = chrono::duration_cast<chrono::milliseconds>(end_buildImage - begin_buildImage).count();

    if (options.verbose) {
        cout << "Saving image: " << buildImageTime << "ms" << endl;
    }
}

void printOutputImageToStdout(VImage& img, Opts& options)
{
    VipsBlob* jpegBuffer = img.jpegsave_buffer(VImage::option()->set("Q", options.quality));

    cout.write(static_cast<const char *>(jpegBuffer->area.data), jpegBuffer->area.length);

    cout.flush();
}

#endif