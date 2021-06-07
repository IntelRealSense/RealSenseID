#include "StreamConverter.h"
#include "Logger.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <cassert>
#include "MetadataDefines.h"

namespace RealSenseID
{
namespace Capture
{
static const StreamAttributes RAW10_1080P_ATTR {1920, 1080, RAW};
static const StreamAttributes MJPEG_1080P_ATTR {1056, 1920, MJPEG};
static const StreamAttributes MJPEG_720P_ATTR {704, 1280, MJPEG};
#ifdef ANDROID
static constexpr int RGB_PIXEL_SIZE = 4;
#else
static constexpr int RGB_PIXEL_SIZE = 3;
#endif
static const char* LOG_TAG = "StreamConverter";

static const StreamAttributes GetStreamAttributesByMode(PreviewMode mode)
{
    switch (mode)
    {
        case PreviewMode::MJPEG_1080P:
            return MJPEG_1080P_ATTR;
        case PreviewMode::MJPEG_720P:
            return MJPEG_720P_ATTR;
        case PreviewMode::RAW10_1080P:
            return RAW10_1080P_ATTR;
        default:
            LOG_ERROR(LOG_TAG, "Got non-legal PreviewMode. using Defualt");
            return MJPEG_1080P_ATTR; 
    }
}

ImageMetadata ExtractMetadataFromMDBuffer(const buffer& buffer, bool to_mili = false)
{
    ImageMetadata md;
    
    int divide_ts = to_mili ? 1000 : 1; // from micro to mili

    if (buffer.size < md_middle_level_size || buffer.data == nullptr)
        return md;

    auto tmp_md = reinterpret_cast<md_middle_level*>(buffer.data);

    if (tmp_md->exposure == 0 && tmp_md->gain == 0) // not valid image
        return md;

    constexpr unsigned int snapshot_jpeg_attribute = (1u << 7);

    md.timestamp = tmp_md->sensor_timestamp / divide_ts;
    md.led = tmp_md->led_status;
    md.projector = tmp_md->projector_status;
    md.sensor_id = tmp_md->sensor_id;
    md.status = tmp_md->status;
    md.is_snapshot = tmp_md->flags & snapshot_jpeg_attribute;

    return md;
}

// Jpeg backend 
static void jpeg_exit_handler(j_common_ptr cinfo)
{
    char msg[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, msg);
    ::jpeg_destroy(cinfo);
    throw std::runtime_error(std::string("jpeg error: ") + msg);
}

void StreamConverter::InitDecompressor()
{
    // replace the default error handler with std:runtime_exception
    _jpeg_dinfo.err = jpeg_std_error(&_jpeg_jerr);
    _jpeg_jerr.error_exit = jpeg_exit_handler;
    jpeg_create_decompress(&_jpeg_dinfo);
}

// StreamConverter
StreamConverter::StreamConverter(PreviewMode mode)
{
    _attributes = GetStreamAttributesByMode(mode);
    _result_image.width = _attributes.width;
    _result_image.height = _attributes.height;
    _result_image.size = (_attributes.format == MJPEG) ? _result_image.width * _result_image.height * RGB_PIXEL_SIZE
                                                      : (_result_image.width * _result_image.height / 4) * 5;
    _result_image.stride = _result_image.size / _result_image.height;
    _result_image.buffer = new unsigned char[_result_image.size];
    InitDecompressor();
}

StreamConverter::~StreamConverter()
{
    ::jpeg_destroy_decompress(&_jpeg_dinfo);

    if (_result_image.buffer != nullptr)
    {
        delete[] _result_image.buffer;
        _result_image.buffer = nullptr;
    }
}

bool StreamConverter::DecodeJpeg(Image* res, buffer frame_buffer)
{    
    ::jpeg_mem_src(&_jpeg_dinfo, frame_buffer.data, frame_buffer.size); 
    auto rc = jpeg_read_header(&_jpeg_dinfo, TRUE);
    if (rc != 1)
    {
        LOG_ERROR(LOG_TAG, "Got invalid jpeg frame");
        assert(false);
        return false;
    }

    if (RGB_PIXEL_SIZE == 4)
        _jpeg_dinfo.out_color_space = JCS_EXT_RGBA;

    ::jpeg_start_decompress(&_jpeg_dinfo);
    auto width = _jpeg_dinfo.output_width;
    auto height = _jpeg_dinfo.output_height;
    if (height > _result_image.height || width > _result_image.height)
    {        
        LOG_ERROR(LOG_TAG, "jpeg decoded dimensions are bigger than expected");
        return false;
    }

    auto pixel_size = _jpeg_dinfo.output_components;
    auto row_stride = width * pixel_size;
    unsigned char* buffer_array[1];
    while (_jpeg_dinfo.output_scanline < _jpeg_dinfo.output_height)
    {        
        buffer_array[0] = res->buffer + (_jpeg_dinfo.output_scanline) * row_stride;
        ::jpeg_read_scanlines(&_jpeg_dinfo, buffer_array, 1);
    }

    if (!::jpeg_finish_decompress(&_jpeg_dinfo))
    {
        LOG_ERROR(LOG_TAG, "jpeg_finish_decompress failed");
        return false;
    }

    res->height = height;
    res->width = width;
    res->stride = row_stride;
    res->size = res->stride * res->height;

    return true;
}

bool StreamConverter::Buffer2Image(Image* res,const buffer& frame_buffer,const buffer& md_buffer)
{
    *res = _result_image;
    switch (_attributes.format) // process image by mode
    {
    case MJPEG:
        try
        {
            res->metadata = ExtractMetadataFromMDBuffer(md_buffer,true);
            bool decode_success = DecodeJpeg(res, frame_buffer);
            return decode_success;
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR(LOG_TAG, "%s", ex.what());
            ::jpeg_destroy_decompress(&_jpeg_dinfo);
            InitDecompressor();
            return false;
        }
        break;
    case RAW:
        res->metadata = ExtractMetadataFromMDBuffer(md_buffer);
        if (res->metadata.timestamp == 0) // don't return non-dumped images
            return false;
        ::memcpy(res->buffer, frame_buffer.data, frame_buffer.size);
        return true;
        break;
    default:
        LOG_ERROR(LOG_TAG, "Unsupported preivew mode");
        return false;
    }    
}

bool StreamConverter::Buffer2Image(Image* res,const buffer& frame_buffer)
{
    buffer dummy;
    return Buffer2Image(res, frame_buffer, dummy);
}

StreamAttributes StreamConverter::GetStreamAttributes()
{
    return _attributes;
}

} // namespace Capture
} // namespace RealSenseID