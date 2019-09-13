#ifndef TIFIG_HEIF
#define TIFIG_HEIF

#include "types.hpp"

using namespace std;

/**
 * Check if image has a grid configuration and return the grid id
 * @param reader
 * @param contextId
 * @return
 */
IdVector findGridItems(const HevcImageFileReader* reader, uint32_t contextId)
{
    IdVector gridItemIds;
    reader->getItemListByType(contextId, "grid", gridItemIds);

    if (gridItemIds.empty()) {
        throw logic_error(
            "Grid configuration not found! tifig currently only supports .heic images created on iOS 11 devices.\n"
            "If you are certain this image was created on iOS 11, please open an issue here:\n\n"
            "https://github.com/monostream/tifig/issues/new"
        );
    }

    return gridItemIds;
}

/**
 * Find thmb reference in metabox
 * @param reader
 * @param contextId
 * @param itemId
 * @return
 */
uint32_t findThumbnailId(const HevcImageFileReader* reader, uint32_t contextId, uint32_t itemId)
{
    IdVector thmbIds;
    reader->getReferencedToItemListByType(contextId, itemId, "thmb", thmbIds);

    if (thmbIds.empty()) {
        throw logic_error("Thumbnail ID not found!");
    }

    return thmbIds.at(0);
}


int findExifHeaderOffset(DataVector &exifData)
{
    string exifPattern = "Exif";
    int exifOffset = -1;
    for (uint64_t i = 0; i < exifData.size() - 4; i++) {
        if (exifData[i+0] == exifPattern[0] &&
            exifData[i+1] == exifPattern[1] &&
            exifData[i+2] == exifPattern[2] &&
            exifData[i+3] == exifPattern[3]) {
            exifOffset = static_cast<int>(i);
            break;
        }
    }

    return exifOffset;
}

/**
 * Extract EXIF data from HEIF
 * @param reader
 * @param contextId
 * @param itemId
 * @return
 */
DataVector extractExifData(HevcImageFileReader* reader, uint32_t contextId, uint32_t itemId)
{
    IdVector exifItemIds;
    DataVector exifData;
    DataVector result;

    reader->getReferencedToItemListByType(contextId, itemId, "cdsc", exifItemIds);

    if (exifItemIds.empty()) {
        cerr << "Warning: Exif Data ID (cdsc) not found!\n";
    } else {
        reader->getItemData(contextId, exifItemIds.at(0), exifData);

        if (exifData.empty()) {
            cerr << "Warning: Exif data is empty\n";
        } else {
            int exifOffset = findExifHeaderOffset(exifData);

            if (exifOffset == -1) {
                cerr <<  "Warning: Exif data not found\n";
            } else {
                uint64_t skipBytes = static_cast<uint64_t>(exifOffset);
                result.insert(result.begin(),  exifData.begin() + skipBytes, exifData.end() - exifOffset);
            }
        }
    }
    return result;
}

#endif