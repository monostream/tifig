#ifndef TIFIG_LOADER
#define TIFIG_LOADER

#include <future>

#include "heif.hpp"
#include "hevc_decode.hpp"
#include "imaging.hpp"

/**
 * Get thumbnail from HEIC image
 * @param reader
 * @param contextId
 * @param gridItemId
 * @return
 */
VImage getThumbnailImage(HevcImageFileReader& reader, uint32_t contextId, uint32_t thmbId)
{
    // Get thumbnail HEVC data
    DataVector hevcData;
    reader.getItemDataWithDecoderParameters(contextId, thmbId, hevcData);

    RgbData rgb = decodeFrame(hevcData);

    // Load image into vips and save as JPEG
    VImage thumbImg = VImage::new_from_memory(rgb.data, rgb.size, rgb.width, rgb.height, 3, VIPS_FORMAT_UCHAR);

    return thumbImg;
}

/**
 * Build image from HEIC grid item
 * @param reader
 * @param contextId
 * @param gridItemId
 * @return
 */
VImage getImage(HevcImageFileReader& reader, uint32_t contextId, uint32_t gridItemId, Opts& options)
{
    GridItem gridItem;
    gridItem = reader.getItemGrid(contextId, gridItemId);

    // Convenience vars
    uint32_t width = gridItem.outputWidth;
    uint32_t height = gridItem.outputHeight;
    uint32_t columns = gridItem.columnsMinusOne + 1;
    uint32_t rows = gridItem.rowsMinusOne + 1;

    if (options.verbose) {
        cout << "Grid is " << width << "x" << height << " pixels in tiles " << columns << "x" << rows << endl;
    }

    // Find master tiles to extract
    IdVector tileItemIds;
    reader.getItemListByType(contextId, "master", tileItemIds);

    uint32_t firstTileId = tileItemIds.at(0);

    // Extract and decode all tiles
    vector<VImage> tiles;
    vector<future<RgbData>> decoderResults;

    for (uint32_t tileItemId : tileItemIds) {
        DataVector hevcData;
        reader.getItemDataWithDecoderParameters(contextId, tileItemId, firstTileId, hevcData);

        if (options.parallel) {
            decoderResults.push_back(async(decodeFrame, hevcData));
        } else {
            RgbData rgb = decodeFrame(hevcData);

            VImage img = VImage::new_from_memory(rgb.data, rgb.size, rgb.width, rgb.height, 3, VIPS_FORMAT_UCHAR);

            tiles.push_back(img);
        }
    }

    if (options.parallel) {
        for (future<RgbData> &futureData: decoderResults) {
            RgbData rgb = futureData.get();

            VImage img = VImage::new_from_memory(rgb.data, rgb.size, rgb.width, rgb.height, 3, VIPS_FORMAT_UCHAR);

            tiles.push_back(img);
        }
    }

    // Stitch tiles together
    VImage image = VImage::new_memory();
    image = image.arrayjoin(tiles, VImage::option()->set("across", (int)columns));
    image = image.extract_area(0, 0, width, height);

    return image;
}

#endif
