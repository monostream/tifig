#include <cxxopts.hpp>
#include <log.hpp>

#include "version.h"
#include "loader.hpp"

/**
 * Sanity check: When you edit a HEIC image on iOS 11 it's saved as JPEG instead of HEIC but still has .heic ending.
 * Starting tifig on such a file, nokia's heif library goes into an endless loop.
 * So check if the file starts with an 'ftyp' box.
 * @param inputFilename
 */
void sanityCheck(const string& inputFilename) {
    ifstream input(inputFilename);
    if (!input.is_open()) {
        throw logic_error("Could not open file " + inputFilename);
    }

    char* bytes = new char[8];
    input.read(bytes, 8);

    if (bytes[4] != 'f' &&
        bytes[5] != 't' &&
        bytes[6] != 'y' &&
        bytes[7] != 'p') {
        throw logic_error("No ftyp box found! This cannot be a HEIF image.");
    }
}

/**
 * Main loop
 * @param inputFilename
 * @param outputFilename
 * @param options
 * @return
 */
int convert(const string& inputFilename, Opts& options)
{
    sanityCheck(inputFilename);

    HevcImageFileReader reader;
    reader.initialize(inputFilename);
    const uint32_t contextId = reader.getFileProperties().rootLevelMetaBoxProperties.contextId;

    // Detect grid
    const IdVector& gridItems = findGridItems(&reader, contextId);

    uint32_t gridItemId = gridItems.at(0);

    VIPS_INIT("tifig");
    avcodec_register_all();

    chrono::steady_clock::time_point begin_encode = chrono::steady_clock::now();

    bool useEmbeddedThumbnail = options.thumbnail;
    bool createOutputThumbnail = options.width > 0;

    // Detect if we safely can use the embedded thumbnail to create output thumbnail
    if (createOutputThumbnail) {

        if (options.width <= 240 && options.height <= 240) {
            useEmbeddedThumbnail = true;
        }
    }

    // Get the actual image content from file
    VImage image;
    if (useEmbeddedThumbnail) {
        const uint32_t thmbId = findThumbnailId(&reader, contextId, gridItemId);
        image = getThumbnailImage(reader, contextId, thmbId);
    } else {
        image = getImage(reader, contextId, gridItemId, options);
    }

    chrono::steady_clock::time_point end_encode = chrono::steady_clock::now();
    long tileEncodeTime = chrono::duration_cast<chrono::milliseconds>(end_encode - begin_encode).count();

    if (options.verbose) {
        cout << "Export & decode HEVC: " << tileEncodeTime << "ms" << endl;
    }

    DataVector exifData = extractExifData(&reader, contextId, gridItemId);
    addExifMetadata(exifData, image);

    if (createOutputThumbnail)
    {
        image = createVipsThumbnail(image, options);
    }

    if (!options.outputPath.empty()) {
        saveOutputImageToFile(image, options);
    }
    else {
        printOutputImageToStdout(image, options);
    }

    vips_shutdown();

    return 0;
}

Opts getTifigOptions(cxxopts::Options& options)
{
    Opts opts = {};

    if (options.count("output"))
        opts.outputPath = options["output"].as<string>();
    if (options.count("width"))
        opts.width = options["width"].as<int>();
    if (options.count("height"))
        opts.height = options["height"].as<int>();
    if (options.count("quality"))
        opts.quality = options["quality"].as<int>();
    if (options.count("crop"))
        opts.crop = true;
    if (options.count("parallel"))
        opts.parallel = true;
    if (options.count("thumbnail"))
        opts.thumbnail = true;
    if (options.count("verbose") && !opts.outputPath.empty())
        opts.verbose = true;

    return opts;
}

void printVersion()
{
    cout << "tifig " << VERSION << endl;
}

int main(int argc, char* argv[])
{
    // Disable colr and pixi boxes unknown warnings from libheif
    Log::getWarningInstance().setLevel(Log::LogLevel::ERROR);

    int retval = 1;

    try {

        cxxopts::Options options(argv[0], "Converts iOS 11 HEIC images to practical formats");

        options.positional_help("input_file [output_file]");

        options.parse_positional(vector<string>{"input", "output"});

        options.add_options()
                ("i, input", "Input HEIF image", cxxopts::value<string>())
                ("o, output", "Output image path", cxxopts::value<string>())
                ("q, quality", "Output JPEG quality", cxxopts::value<int>()
                        ->default_value("90")->implicit_value("90"))
                ("v, verbose", "Verbose output", cxxopts::value<bool>())
                ("w, width", "Width of output image", cxxopts::value<int>())
                ("h, height", "Height of output image", cxxopts::value<int>())
                ("c, crop", "Smartcrop image to fit given size", cxxopts::value<bool>())
                ("p, parallel", "Decode tiles in parallel", cxxopts::value<bool>())
                ("t, thumbnail", "Use embedded thumbnail", cxxopts::value<bool>())
                ("version", "Show tifig version ")
                ;

        options.parse(argc, argv);

        if (options.count("version")) {
            printVersion();
            retval = 0;
        } else if (options.count("input")) {
            string inputFileName = options["input"].as<string>();

            Opts tifigOptions = getTifigOptions(options);

            chrono::steady_clock::time_point begin = chrono::steady_clock::now();

            retval = convert(inputFileName, tifigOptions);

            chrono::steady_clock::time_point end = chrono::steady_clock::now();
            long duration = chrono::duration_cast<chrono::milliseconds>(end - begin).count();

            if (tifigOptions.verbose) {
                cout << "Total Time: " << duration << "ms" << endl;
            }
        } else {
            cerr << options.help() << endl;
        }
    }
    catch (const cxxopts::OptionException& oe) {
        cerr << "error parsing options: " << oe.what() << endl;
    }
    catch (const FileReaderException& fre) {
        cerr << "Could not read HEIF image: " << fre.what() << endl;
    }
    catch (const logic_error& le) {
        cerr << le.what() << endl;
    }
    catch (exception& e) {
        cerr << "Conversion failed:" << e.what() << endl;
    }

    return retval;
}

