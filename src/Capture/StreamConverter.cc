#include "StreamConverter.h"
#include "Logger.h"
#include <cstring>
#include <algorithm>

namespace RealSenseID
{
namespace Capture
{
static const char* LOG_TAG = "StreamConverter";

static void Clamp(double* num)
{
    if (*num < 0)
        *num = 0;
    else if (*num > 255)
        *num = 255;
}

static void SetRgb(int y, int cb, int cr, double* r, double* g, double* b)
{
    *r = y + (1.4065 * (cr - 128));
    *g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
    *b = y + (1.7790 * (cb - 128));

    Clamp(r);
    Clamp(g);
    Clamp(b);
}

void Yuv2Rgb(Image* res, unsigned char* yuyv_image, unsigned int buffer_size)
{
    unsigned char* rgb_image = res->buffer;

    double r;
    double g;
    double b;

    for (unsigned int i = 0, j = 0; i < res->size; j += 4)
    {
        SetRgb(yuyv_image[j], yuyv_image[j + 1], yuyv_image[j + 3], &r, &g, &b);

        rgb_image[i++] = static_cast<unsigned char>(r);
        rgb_image[i++] = static_cast<unsigned char>(g);
        rgb_image[i++] = static_cast<unsigned char>(b);
        if (VGA_PIXEL_SIZE == RGBA_PIXEL_SIZE)
            rgb_image[i++] = 255;
        
        SetRgb(yuyv_image[j + 2], yuyv_image[j + 1], yuyv_image[j + 3], &r, &g, &b);

        rgb_image[i++] = static_cast<unsigned char>(r);
        rgb_image[i++] = static_cast<unsigned char>(g);
        rgb_image[i++] = static_cast<unsigned char>(b);
        if (VGA_PIXEL_SIZE == RGBA_PIXEL_SIZE)
            rgb_image[i++] = 255;
    }
}


// Handling RAW IMAGE
// IsDumpedImage returns true iff there is a valid timestamp
bool IsValidDumpedImage(const unsigned char* buffer)
{
    const int32_t* time_stamp = reinterpret_cast<const int32_t*>(buffer);
    return *time_stamp != 0;
}

// IsDumpedImage returns true iff facerect is legal
bool IsValidFaceRect(ImageMetadata metadata, unsigned int img_width, unsigned int img_height)
{
    return (metadata.face_rect.x > 0 && metadata.face_rect.y > 0 &&
            metadata.face_rect.x + metadata.face_rect.width < img_height &&
            metadata.face_rect.y + metadata.face_rect.height < img_width);
};

// Extracts metadata from buffer
ImageMetadata ExtractMetadata(const unsigned char* buffer,unsigned int size)
{
    ImageMetadata md;
    if (size < 6 || !IsValidDumpedImage(buffer))
        return md;

    md.timestamp = *(reinterpret_cast<const int32_t*>(buffer));
    md.status = *(buffer + 4);

    unsigned char boolean_data = *(buffer + 5);
    md.sensor_id = (boolean_data)&1;
    md.led = (boolean_data) & (1 << 1);
    md.projector = (boolean_data) & (1 << 2);
    
    bool valid_face_rect_flag = *(buffer + 6);
    if (valid_face_rect_flag)
    {
        md.face_rect.x = *(reinterpret_cast<const int32_t*>(buffer + 7)) * 2;
        md.face_rect.y = *(reinterpret_cast<const int32_t*>(buffer + 11)) * 2;
        md.face_rect.width = *(reinterpret_cast<const int32_t*>(buffer + 15)) * 2;
        md.face_rect.height = *(reinterpret_cast<const int32_t*>(buffer + 19)) * 2;
    }
    return md;
}

static uint8_t Raw8Value(const uint8_t* src, int baseoff, int mod5)
{
    if (mod5 >= 4)
    {
        LOG_ERROR(LOG_TAG,"%s(): Invalid index %d!", __FUNCTION__, mod5);
        return 0;
    }

    return *(src + baseoff);
}

void RotatedRaw2Rgb(Image* res, unsigned char* buffer, unsigned int buffer_size)
{
    unsigned int src_height = res->height, src_width = res->width;
    unsigned int dst_height = src_width, dst_width = src_height; // rotating image 

    if (((src_height * src_width / 4) * 5) != buffer_size) // check for valid w10 image 10bpp
    {
        return;
    }

    unsigned int line = buffer_size / src_height;

    const unsigned char* src = buffer;
    unsigned char* dst = res->buffer;

    const int palette = 128;

    unsigned int src_x = 0, src_y = 0;
    unsigned int dst_x = dst_width -1 , dst_y = dst_height -1; 
    unsigned int i;

    for (i = 0; i < buffer_size; i++)
    {
        int p[8];
        uint8_t hn, vn, di;
        uint8_t br0, br1, g;
        int mode;
        int hnext = 1, hprev = 1;

        /* skip every 5th byte */
        switch (i % 5)
        {
        case 0:
            hprev = 2;
            break;
        case 3:
            hnext = 2;
            break;
        case 4:
            continue;
        }

        /* Setup offsets to this pixel's neighbours. */
        p[0] = i - line - hprev;
        p[1] = i - line;
        p[2] = i - line + hnext;
        p[3] = i - hprev;
        p[4] = i + hnext;
        p[5] = i + line - hprev;
        p[6] = i + line;
        p[7] = i + line + hnext;

        /* Swap offsets if they are out of bounds. */
        if (!src_y)
        {
            p[0] = p[5];
            p[1] = p[6];
            p[2] = p[7];
        }
        else if (src_y == src_height - 1)
        {
            p[5] = p[0];
            p[6] = p[1];
            p[7] = p[2];
        }

        if (!src_x)
        {
            p[0] = p[2];
            p[3] = p[4];
            p[5] = p[7];
        }
        else if (src_x == src_width - 1)
        {
            p[2] = p[0];
            p[4] = p[3];
            p[7] = p[5];
        }

        /* Average matching neighbours. */
        hn = ((Raw8Value(src, p[3], p[3] % 5) + Raw8Value(src, p[4], p[4] % 5)) / 2);
        vn = ((Raw8Value(src, p[1], p[1] % 5) + Raw8Value(src, p[6], p[6] % 5)) / 2);
        di = ((Raw8Value(src, p[0], p[0] % 5) + Raw8Value(src, p[2], p[2] % 5) + Raw8Value(src, p[5], p[5] % 5) +
               Raw8Value(src, p[7], p[7] % 5)) /
              4);

        /* Calculate RGB */
        if (palette == 128)
            mode = (src_x + src_y) & 0x01;
        else
            mode = ~(src_x + src_y) & 0x01;

        if (mode)
        {
            g = Raw8Value(src, i, i % 5);
            if (src_y & 0x01)
            {
                br1 = hn;
                br0 = vn;
            }
            else
            {
                br1 = vn;
                br0 = hn;
            }
        }
        else if (src_y & 0x01)
        {
            br1 = Raw8Value(src, i, i % 5);
            g = (vn + hn) / 2;
            br0 = di;
        }
        else
        {
            br0 = Raw8Value(src, i, i % 5);
            g = (vn + hn) / 2;
            br1 = di;
        }

        unsigned int cur_rgb_pixel_offset = VGA_PIXEL_SIZE * (dst_y * dst_width + dst_x);
        *(dst + cur_rgb_pixel_offset) = br1;
        *(dst + cur_rgb_pixel_offset + 1) = g;
        *(dst + cur_rgb_pixel_offset + 2) = br0;
        
        if (VGA_PIXEL_SIZE == RGBA_PIXEL_SIZE)
        {
            *(dst + 3) = 255;
        }
        
        dst_y--;
        src_x++;

        /* Move to the next line*/
        if (src_x == src_width)
        {
            src_x = 0;
            src_y++;
            dst_y = dst_height - 1;
            dst_x--;
        }
    }

    // change image attr to match the convertion
    res->height = dst_height;
    res->width = dst_width;
    res->stride = res->size / res->height;
}

void StreamConverter::InitStream(unsigned int width, unsigned int height, PreviewMode mode)
{
    _mode = mode;
    _attr.width = width;
    _attr.height = height;
    _attr.size = (_mode != PreviewMode::Dump) ? _attr.width * _attr.height * VGA_PIXEL_SIZE
                                              : (_attr.width * _attr.height / 4) * 5;
    _attr.stride = _attr.size / _attr.height;
    _attr.buffer = new unsigned char[_attr.size];
}

StreamConverter::~StreamConverter()
{
    if (_attr.buffer != nullptr)
    {
        delete[] _attr.buffer;
        _attr.buffer = nullptr;
    }
    
}

bool StreamConverter::Buffer2Image(Image* res, unsigned char* src_buffer, unsigned int src_buffer_size)
{
    *res = _attr;
    switch (_mode) // process image by mode
    {
    case PreviewMode::VGA:
        Yuv2Rgb(res, src_buffer, src_buffer_size);
        break;
    case PreviewMode::FHD_Rect:
        res->metadata = ExtractMetadata(src_buffer, src_buffer_size);
        if (!IsValidFaceRect(res->metadata, res->width, res->height))
            return false;
        RotatedRaw2Rgb(res, src_buffer, src_buffer_size);
        break;
    case PreviewMode::Dump:
        if (!IsValidDumpedImage(src_buffer))
            return false;
        res->metadata = ExtractMetadata(src_buffer, res->size);
        memcpy((void*)res->buffer, (void*)src_buffer, src_buffer_size);
        break;
    }
    return true;
}

} // namespace Capture
} // namespace RealSenseID