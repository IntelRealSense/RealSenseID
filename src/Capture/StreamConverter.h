#pragma once
#include "RealSenseID/Preview.h"
#include <stdio.h> // needed for jpeglib's FILE* usage
#include "jpeglib.h"
#include <memory>
#include <functional>

namespace RealSenseID
{
namespace Capture
{

static constexpr int RGB_PIXEL_SIZE = 3;

enum StreamFormat
{
    MJPEG,
    RAW
};

struct StreamAttributes
{
    unsigned int width = 0;
    unsigned int height = 0;
    StreamFormat format = MJPEG;
};

struct buffer
{
    unsigned char* data = nullptr;
    unsigned int size = 0;
    unsigned int offset = 0;
};

class StreamConverter
{
public:
    explicit StreamConverter(PreviewConfig config);
    ~StreamConverter();    
    bool Buffer2Image(Image* res,const buffer& frame_buffer,const buffer& metadata_buffer);
    StreamAttributes GetStreamAttributes();

private:
    StreamAttributes _attributes;
    Image _result_image; 
    bool _portrait_mode;
    // jpeg structs
    jpeg_error_mgr _jpeg_jerr;
    jpeg_decompress_struct _jpeg_dinfo;

    void InitDecompressor();
    bool DecodeJpeg(Image* res, buffer frame_buffer);
};
} // namespace Capture
} // namespace RealSenseID
