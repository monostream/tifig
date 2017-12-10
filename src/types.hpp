#ifndef TIFIG_TYPES
#define TIFIG_TYPES

#include <hevcimagefilereader.hpp>

typedef ImageFileReaderInterface::DataVector DataVector;
typedef ImageFileReaderInterface::IdVector IdVector;
typedef ImageFileReaderInterface::GridItem GridItem;
typedef ImageFileReaderInterface::FileReaderException FileReaderException;

struct RgbData
{
    uint8_t* data = nullptr;
    size_t size = 0;
    int width = 0;
    int height = 0;
};

struct Opts
{
    std::string outputPath = "";
    int width = 0;
    int height = 0;
    int quality = 90;
    bool crop = false;
    bool parallel = false;
    bool thumbnail = false;
    bool verbose = false;
};

#endif