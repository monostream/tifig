#include <chrono>
#include <cxxopts.hpp>
#include <exif.h>
#include <hevcimagefilereader.hpp>
#include <log.hpp>
#include <vips/vips8>
#include <unistd.h>

using namespace std;
using namespace vips;


static bool VERBOSE = false;
static int QUALITY = 90;
static string TEMP_DIR;

/**
 * Check if image has a grid configuration and return the grid id
 * @param reader
 * @param contextId
 * @return
 */
ImageFileReaderInterface::IdVector findGridItems(const HevcImageFileReader *reader, uint32_t contextId) {
    ImageFileReaderInterface::IdVector gridItemIds;
    reader->getItemListByType(contextId, "grid", gridItemIds);

    if (gridItemIds.size() == 0) {
        throw logic_error("No grid items founds!");
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
uint32_t findThumbnailId(const HevcImageFileReader *reader, uint32_t contextId, uint32_t itemId) {
    ImageFileReaderInterface::IdVector thmbIds;
    reader->getReferencedToItemListByType(contextId, itemId, "thmb", thmbIds);

    if (thmbIds.size() == 0) {
        throw logic_error("Thumbnail ID not found!");
    }

    return thmbIds.at(0);
}

/**
 * Configure decoder to extract HEVC stream
 * @param reader
 * @param contextId
 * @param itemId
 * @return
 */
ImageFileReaderInterface::DataVector getDecoderParams(const HevcImageFileReader *reader,
                                                      uint32_t contextId,
                                                      uint32_t itemId) {
    HevcImageFileReader::ParameterSetMap parameterSet;
    reader->getDecoderParameterSets(contextId, itemId, parameterSet);
    string codeType = reader->getDecoderCodeType(contextId, itemId);
    ImageFileReaderInterface::DataVector parametersData;
    if ((codeType == "hvc1") || (codeType == "lhv1")) {
        // VPS (HEVC specific)
        parametersData.insert(parametersData.end(), parameterSet.at("VPS").begin(), parameterSet.at("VPS").end());
    }

    if ((codeType == "avc1") || (codeType == "hvc1") || (codeType == "lhv1")) {
        // SPS and PPS
        parametersData.insert(parametersData.end(), parameterSet.at("SPS").begin(), parameterSet.at("SPS").end());
        parametersData.insert(parametersData.end(), parameterSet.at("PPS").begin(), parameterSet.at("PPS").end());
    } else {
        // No other code types supported
        throw logic_error("Image encoded wit" + codeType + " is not supported");
    }

    return parametersData;
}

/**
 * Extract HEVC data stream with decoder options
 * @param reader
 * @param decoderParams
 * @param contextId
 * @param itemId
 * @return
 */
ImageFileReaderInterface::DataVector extractHEVCData(HevcImageFileReader *reader,
                                                     const ImageFileReaderInterface::DataVector *decoderParams,
                                                     uint32_t contextId,
                                                     uint32_t itemId) {
    ImageFileReaderInterface::DataVector dataWithDecoderParams;
    ImageFileReaderInterface::DataVector itemData;

    dataWithDecoderParams.insert(dataWithDecoderParams.end(), decoderParams->begin(), decoderParams->end());

    reader->getItemData(contextId, itemId, itemData);

    dataWithDecoderParams.insert(dataWithDecoderParams.end(), itemData.begin() + 1, itemData.end());

    return dataWithDecoderParams;
}

/**
 * Extract HEVC data stream to file
 * @param reader
 * @param decoderParams
 * @param tileFileName
 * @param contextId
 * @param tileItemId
 * @return
 */
void writeHevcDataToFile(ImageFileReaderInterface::DataVector *hevcData, string hevcFileName)
{
    ofstream hevcFile(hevcFileName);
    if (!hevcFile.is_open()) {
        throw logic_error("Could not open " + hevcFileName + " for writing HEVC");
    }

    hevcFile.write((char *) &((*hevcData)[0]), hevcData->size());
    if (hevcFile.bad()) {
        throw logic_error("failed to write to " + hevcFileName);
    }

    hevcFile.close();
}

/**
 * Extract EXIF data from HEIF
 * @param reader
 * @param contextId
 * @param itemId
 * @return
 */
easyexif::EXIFInfo extractExifData(HevcImageFileReader *reader, uint32_t contextId, uint32_t itemId)
{
    ImageFileReaderInterface::IdVector exifItemIds;
    ImageFileReaderInterface::DataVector exifData;

    reader->getReferencedToItemListByType(contextId, itemId, "cdsc", exifItemIds);

    if (exifItemIds.size() == 0) {
        throw logic_error("Exif Data ID (cdsc) not found!");
    }

    reader->getItemData(contextId, exifItemIds.at(0), exifData);

    easyexif::EXIFInfo exifInfo;
    int parseRet = exifInfo.parseFromEXIFSegment(&exifData[4], exifData.size() - 4);

    if (parseRet != 0) {
        throw logic_error("Failed to parse EXIF data!");
    }

    return exifInfo;
}

/**
 * Build full image from tiles
 * @param width
 * @param height
 * @param columns
 * @param tiles
 */
VImage buildFullImage(int width, int height, int columns, vector<string> tiles)
{
    const int tileSize = 512; // FIXME: Do we really need to hardcode this?;

    VImage combined = VImage::new_matrix(width, height);

    int offsetX = 0;
    int offsetY = 0;

    for (int i = 0; i < tiles.size(); i++) {
        VImage in = VImage::new_from_file(tiles[i].c_str());

        combined = combined.insert(in, offsetX, offsetY);

        if ((i + 1) % columns == 0) {
            offsetY += tileSize;
            offsetX = 0;
        }
        else {
            offsetX += tileSize;
        }
    }

    return combined;
}

/**
 *
 * @param inputFilename
 * @param outputFilename
 * @return
 */
int exportThumbnail(string inputFilename, string outputFilename) {

    HevcImageFileReader reader;
    reader.initialize(inputFilename);
    const uint32_t contextId = reader.getFileProperties().rootLevelMetaBoxProperties.contextId;

    // Detect grid
    const auto &gridItems = findGridItems(&reader, contextId);

    uint32_t gridItemId = gridItems.at(0);

    // Find Thumbnail ID
    const uint32_t thmbId = findThumbnailId(&reader, contextId, gridItemId);

    // Extract EXIF data;
    easyexif::EXIFInfo exifInfo = extractExifData(&reader, contextId, gridItemId);

    // Configure decoder
    const auto& decoderParams = getDecoderParams(&reader, contextId, thmbId);


    // Extract & decode thumbnail
    ImageFileReaderInterface::DataVector hevcData = extractHEVCData(&reader, &decoderParams, contextId, thmbId);

    string hevcFile = TEMP_DIR + "/thmb.hevc";
    string jpegFile = TEMP_DIR + "/thmb.jpg";

    writeHevcDataToFile(&hevcData, hevcFile);

    string ffmpegCall = "ffmpeg -i " + hevcFile + " -loglevel panic -frames:v 1 -y " + jpegFile;

    int ffmpegResult = system(ffmpegCall.c_str());

    if (ffmpegResult != 0)
    {
        cerr << "Decoding thumbnail failed" << endl;
        return ffmpegResult;
    }

    // Load image and save with orientation

    VImage result = VImage::new_from_file(jpegFile.c_str());

    result.set(VIPS_META_ORIENTATION, exifInfo.Orientation);

    result.jpegsave(const_cast<char *>(outputFilename.c_str()), VImage::option()->set("Q", QUALITY));

    return 0;
}

/**
 *
 * @param inputFilename
 * @param outputFilename
 * @return
 */
int convertToJpeg(string inputFilename, string outputFilename) {

    HevcImageFileReader reader;

    reader.initialize(inputFilename);
    const uint32_t contextId = reader.getFileProperties().rootLevelMetaBoxProperties.contextId;

    // Detect grid
    const auto &gridItems = findGridItems(&reader, contextId);

    uint32_t gridItemId = gridItems.at(0);
    ImageFileReaderInterface::GridItem gridItem;
    gridItem = reader.getItemGrid(contextId, gridItemId);

    // Convenience vars
    uint32_t width = gridItem.outputWidth;
    uint32_t height = gridItem.outputHeight;
    uint32_t columns = gridItem.columnsMinusOne + 1;
    uint32_t rows = gridItem.rowsMinusOne + 1;

    if (VERBOSE) {
        cout << "Grid is " << width << "x" << height << " pixels in tiles " << columns << "x" << rows << endl;
    }

    // Extract EXIF data;
    easyexif::EXIFInfo exifInfo = extractExifData(&reader, contextId, gridItemId);

    // Find master tiles to extract
    ImageFileReaderInterface::IdVector tileItemIds;
    reader.getItemListByType(contextId, "master", tileItemIds);

    // Configure decoder
    const ImageFileReaderInterface::DataVector decoderParams = getDecoderParams(&reader, contextId, tileItemIds.at(0));

    // Extract and decode all tiles
    chrono::steady_clock::time_point begin_encode = chrono::steady_clock::now();

    string ffmpegInvokes;

    vector<string> tilePaths;

    for (auto &tileItemId : tileItemIds) {
        string hevcTile = TEMP_DIR + "/tile" + to_string(tileItemId) + ".hevc";
        string jpegTile =  TEMP_DIR + "/tile" + to_string(tileItemId) + ".jpg";

        ImageFileReaderInterface::DataVector hevcData = extractHEVCData(&reader, &decoderParams, contextId, tileItemId);
        writeHevcDataToFile(&hevcData, hevcTile);

        string ffmpegCall = "ffmpeg -i " + hevcTile + " -loglevel panic -frames:v 1 -q:v 1 -y " + jpegTile + " & ";

        tilePaths.push_back(jpegTile);

        ffmpegInvokes += ffmpegCall;
    }

    ffmpegInvokes += "wait";

    int ffmpegResult = system(ffmpegInvokes.c_str());

    if (ffmpegResult != 0)
    {
        cerr << "Decoding tiles failed" << endl;
        return ffmpegResult;
    }

    chrono::steady_clock::time_point end_encode = chrono::steady_clock::now();
    long tileEncodeTime = chrono::duration_cast<chrono::milliseconds>(end_encode - begin_encode).count();

    if (VERBOSE) {
        cout << "Export & encode tiles " << tileEncodeTime << "ms" << endl;
    }


    // Stitch tiles together

    chrono::steady_clock::time_point begin_buildImage = chrono::steady_clock::now();

    VImage result = buildFullImage(width, height, columns, tilePaths);

    result.set(VIPS_META_ORIENTATION, exifInfo.Orientation);

    result.jpegsave(const_cast<char *>(outputFilename.c_str()), VImage::option()->set("Q", QUALITY));

    chrono::steady_clock::time_point end_buildImage = chrono::steady_clock::now();
    long buildImageTime = chrono::duration_cast<chrono::milliseconds>(end_buildImage - begin_buildImage).count();

    if (VERBOSE) {
        cout << "Building image " << buildImageTime << "ms" << endl;
    }

    return 0;
}

int checkDependencies() {

    // TODO: get rid of all binary depenecies :)
    if (system("which ffmpeg > /dev/null 2>&1")) {
        cerr << "Requirement not found: missing ffmpeg" << endl;
        return -1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    Log::getWarningInstance().setLevel(Log::LogLevel::ERROR);

    int dependencies = checkDependencies();
    if (dependencies != 0) return dependencies;

    bool thumb = false;
    int retval = -1;

    VIPS_INIT(argv[0]);

    string tempTemplate = "/tmp/heif.XXXXXX";
    TEMP_DIR = mkdtemp(const_cast<char *>(tempTemplate.c_str()));

    try {

        cxxopts::Options options(argv[0], "Convert HEIF images to JPEG");

        options.positional_help("input_file output_file");

        options.parse_positional(vector<string>{"input", "output"});

        options.add_options()
                ("i,input", "Input HEIF image", cxxopts::value<string>())
                ("o,output", "Output JPEG image", cxxopts::value<string>())
                ("q,quality", "Output JPEG quality (1-100) default 90", cxxopts::value<int>(QUALITY))
                ("v,verbose", "Verbose output", cxxopts::value<bool>(VERBOSE))
                ("t,thumbnail", "Export thumbnail", cxxopts::value<bool>(thumb))
                ;

        options.parse(argc, argv);

        if (options.count("input") && options.count("output")) {

            string inputFileName = options["input"].as<string>();
            string outputFileName = options["output"].as<string>();


            chrono::steady_clock::time_point begin = chrono::steady_clock::now();

            if (thumb) {
                retval = exportThumbnail(inputFileName, outputFileName);
            } else {
                retval = convertToJpeg(inputFileName, outputFileName);
            }

            chrono::steady_clock::time_point end = chrono::steady_clock::now();
            int  duration = chrono::duration_cast<chrono::milliseconds>(end - begin).count();

            if (VERBOSE) {
                cout << "Total Time " << duration << "ms" << endl;
            }

        }
        else {
            cout << options.help() << endl;
        }


    }
    catch (const cxxopts::OptionException& oe) {
        cout << "error parsing options: " << oe.what() << endl;
    }
    catch (const ImageFileReaderInterface::FileReaderException& fre) {
        cerr << "Could not read HEIF image: " << fre.what() << endl;
    }
    catch (const logic_error& le) {
        cerr << le.what() << endl;
    }

    int rm = system(("rm -r " + TEMP_DIR).c_str());

    vips_shutdown();

    return rm != 0 ? rm : retval;
}

