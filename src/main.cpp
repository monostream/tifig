#include <chrono>
#include <cxxopts.hpp>
#include <exif.h>
#include <hevcimagefilereader.hpp>
#include <log.hpp>
#include <vips/vips8>
#include <unistd.h>


extern "C" {
    #include <libavutil/opt.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/common.h>
};

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

    dataWithDecoderParams.insert(dataWithDecoderParams.end(), itemData.begin(), itemData.end());

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
 * Decode HEVC Frame using libav
 * @param hevcData
 * @return
 */
AVFrame* decodeHEVCFrame(ImageFileReaderInterface::DataVector& hevcData) {
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    AVCodecContext *c;
    AVFrame *frame;
    AVPacket avpkt;

    c = avcodec_alloc_context3(codec);

    if (!c) {
        throw logic_error("Could not allocate video codec context");
    }

    frame = av_frame_alloc();
    if (!frame) {
        throw logic_error("Could not allocate frame");
    }

    if (avcodec_open2(c, codec, NULL) < 0) {
        throw logic_error("Could not open codec");
    }

    av_init_packet(&avpkt);

    avpkt.size = hevcData.size();
    avpkt.data = &hevcData[0];

    int success;

    int len = avcodec_decode_video2(c, frame, &success, &avpkt);

    if (len < 0 || !success) {
        throw logic_error("Failed to decode frame");
    }

    avcodec_close(c);
    av_free(c);

    return frame;
}

/**
 * Encode AVFrame as JPEG Image
 * @param frame
 * @return
 */
AVPacket* encodeAVFrameToJPEG(AVFrame * frame) {
    AVCodec *jpegCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    AVCodecContext *jpegContext = avcodec_alloc_context3(jpegCodec);
    AVFrame *jf = av_frame_alloc();

    jpegContext->width = frame->width;
    jpegContext->height = frame->height;
    jpegContext->pix_fmt = AV_PIX_FMT_YUVJ420P;
    jpegContext->time_base.num = 1;
    jpegContext->time_base.den = 1;

    if (avcodec_open2(jpegContext, jpegCodec, NULL) < 0) {
        throw logic_error("Could not open JPEG codec");
    }

    AVPacket* jpegPacket = new AVPacket();
    av_init_packet(jpegPacket);
    jpegPacket->data = NULL;
    jpegPacket->size = 0;

    int success;
    int length = avcodec_encode_video2(jpegContext, jpegPacket, frame, &success);

    if (length < 0 || !success) {
        throw logic_error("Failed to encode frame to JPEG");
    }

    avcodec_close(jpegContext);
    av_free(jpegContext);

    return jpegPacket;
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
VImage buildFullImage(int width, int height, int columns, vector<AVPacket> tiles)
{
    const int tileSize = 512; // FIXME: Do we really need to hardcode this?;

    VImage combined = VImage::new_matrix(width, height);

    int offsetX = 0;
    int offsetY = 0;

    for (int i = 0; i < tiles.size(); i++) {
        VImage in = VImage::new_from_buffer(tiles[i].data, tiles[i].size, NULL);
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

    // Decode HEVC Frame
    AVFrame* frame = decodeHEVCFrame(hevcData);

    // Encode frame to jpeg
    AVPacket *jpegData = encodeAVFrameToJPEG(frame);

    // Read data with vips
    VImage thumbJpg = VImage::new_from_buffer(jpegData->data, jpegData->size, NULL);

    cout << thumbJpg.width() << "x" << thumbJpg.height() << " pixels" << endl;

    thumbJpg.set(VIPS_META_ORIENTATION, exifInfo.Orientation);
    thumbJpg.jpegsave(const_cast<char *>(outputFilename.c_str()), VImage::option()->set("Q", QUALITY));

    // cleanup
    av_free(frame);

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

    vector<AVPacket> tileJpegs;

    for (auto &tileItemId : tileItemIds) {

        // ToDo: Parallelize
        ImageFileReaderInterface::DataVector hevcData = extractHEVCData(&reader, &decoderParams, contextId, tileItemId);

        AVFrame* frame = decodeHEVCFrame(hevcData);

        AVPacket *jpegData = encodeAVFrameToJPEG(frame);

        tileJpegs.push_back(*jpegData);
    }


    chrono::steady_clock::time_point end_encode = chrono::steady_clock::now();
    long tileEncodeTime = chrono::duration_cast<chrono::milliseconds>(end_encode - begin_encode).count();

    if (VERBOSE) {
        cout << "Export & encode tiles " << tileEncodeTime << "ms" << endl;
    }


    // Stitch tiles together

    chrono::steady_clock::time_point begin_buildImage = chrono::steady_clock::now();

    VImage result = buildFullImage(width, height, columns, tileJpegs);

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

    avcodec_register_all();

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

