#include "StreamConverter.h"
#include "Logger.h"
#include <cstring>
#include <algorithm>
#include "MetadataDefines.h"

namespace RealSenseID
{
namespace Capture
{
static const StreamAttributes RAW10_1080P_ATTR {1920, 1080, RAW};
static const StreamAttributes MJPEG_1080P_ATTR {1056, 1920, MJPEG};
static const StreamAttributes MJPEG_720P_ATTR {704, 1280, MJPEG};
static const StreamAttributes MJPEG_1080P_HORIZON_ATTR {1920, 1056, MJPEG};
static const StreamAttributes MJPEG_720P_HORIZON_ATTR {1280, 704, MJPEG};
#define MD_CAPTURE_INFO_VER 0x80081005

static const char* LOG_TAG = "StreamConverter";

static const StreamAttributes GetStreamAttributesByMode(PreviewConfig config)
{
    switch (config.previewMode)
    {
    case PreviewMode::MJPEG_1080P:
        return config.portraitMode ? MJPEG_1080P_ATTR : MJPEG_1080P_HORIZON_ATTR;
    case PreviewMode::MJPEG_720P:
        return config.portraitMode ? MJPEG_720P_ATTR : MJPEG_720P_HORIZON_ATTR;
    case PreviewMode::RAW10_1080P:
        return RAW10_1080P_ATTR;
    default:
        LOG_ERROR(LOG_TAG, "Got non-legal PreviewMode. using Default");
        return MJPEG_1080P_ATTR;
    }
}

static ImageMetadata ExtractMetadataFromMDBuffer(const buffer& buffer, bool to_milli)
{
    ImageMetadata md;   
    int divide_ts = to_milli ? 1000 : 1; // from micro to milli

    if (buffer.size < md_middle_level_size || buffer.data == nullptr)
        return md;

    auto tmp_md = reinterpret_cast<md_middle_level*>(buffer.data + buffer.offset);

    if (tmp_md->ver != MD_CAPTURE_INFO_VER)
    {
        LOG_ERROR(LOG_TAG, "Metadata version doesn't match. Expected: %x, found: %x", MD_CAPTURE_INFO_VER, tmp_md->ver);
        return md;
    }

    if (tmp_md->exposure == 0 && tmp_md->gain == 0)  
    {
        LOG_DEBUG(LOG_TAG, "Ignoring sync frame (exposure = 0 && gain = 0)");
        return md;
    }

    constexpr unsigned int snapshot_jpeg_attribute = (1u << 7);

    md.timestamp = static_cast<int>(tmp_md->sensor_timestamp / divide_ts);
    md.exposure = tmp_md->exposure;
    md.gain = tmp_md->gain;
    md.led = static_cast<char>(tmp_md->led_status);
    md.sensor_id = tmp_md->sensor_id;
    md.status = tmp_md->status;
    md.is_snapshot = (tmp_md->flags & snapshot_jpeg_attribute) ? 1 : 0;

    // Enable for debugging metadata. (Too noisy)
    /*
    LOG_DEBUG(LOG_TAG, "[Frame metadata] timestamp: %i, exposure: %i, gain: %i, led: %i, "
                       "sensor: %i, status: %i, snapshot: %i",
              md.timestamp, md.exposure, md.gain, md.led, md.sensor_id, md.status, md.is_snapshot);
    */
    return md;
}

// StreamConverter
StreamConverter::StreamConverter(PreviewConfig config) : _portrait_mode(config.portraitMode)
{
#ifdef _WIN32
    _jpeg_decoder = std::make_unique<JPEGWICDecoder>();
#else
    _jpeg_decoder = std::make_unique<JPEGTurboDecoder>();
#endif
    _attributes = GetStreamAttributesByMode(config);
    _result_image.width = _attributes.width;
    _result_image.height = _attributes.height;
    _result_image.size = (_attributes.format == MJPEG) ? _result_image.width * _result_image.height * RGB_PIXEL_SIZE
                                                       : (_result_image.width * _result_image.height / 4) * 5;
    _result_image.stride = _result_image.size / _result_image.height;
    _result_image.buffer = new unsigned char[_result_image.size];
}

StreamConverter::~StreamConverter()
{
    if (_result_image.buffer != nullptr)
    {
        delete[] _result_image.buffer;
        _result_image.buffer = nullptr;
    }
}

bool StreamConverter::Buffer2Image(Image* res,const buffer& frame_buffer,const buffer& md_buffer)
{
    *res = _result_image;
    switch (_attributes.format) // process image by mode
    {
    case MJPEG:
        try
        {  
            res->metadata = ExtractMetadataFromMDBuffer(md_buffer, true /* convert to millis */);
            return _jpeg_decoder->DecodeJpeg(res, frame_buffer, _result_image.height, _result_image.width);
        }
        catch (const std::exception& ex)
        {
            LOG_WARNING(LOG_TAG, "Buffer2Image: %s", ex.what());
            return false;
        }
        break;
    case RAW:
        res->metadata = ExtractMetadataFromMDBuffer(md_buffer, false /* keep in micros */);
        if (res->metadata.timestamp == 0)  // don't return non-dumped images
        {
            LOG_DEBUG(LOG_TAG, "Frame timestamp = 0. Discarded frame.");
            return false;
        }
        ::memcpy(res->buffer, frame_buffer.data, frame_buffer.size);
        return true;
        break;
    default:
        LOG_ERROR(LOG_TAG, "Unsupported preview mode");
        return false;
    }    
}

StreamAttributes StreamConverter::GetStreamAttributes()
{
    return _attributes;
}

} // namespace Capture
} // namespace RealSenseID