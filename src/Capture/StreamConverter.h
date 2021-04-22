#pragma once
#include "RealSenseID/Preview.h"

namespace RealSenseID
{
namespace Capture
{
static const unsigned int RAW_WIDTH = 1920;
static const unsigned int RAW_HEIGHT = 1080;
static const unsigned int VGA_WIDTH = 704;
static const unsigned int VGA_HEIGHT = 1280;
static const unsigned int YUV_PIXEL_SIZE = 2;
static const unsigned int RGB_PIXEL_SIZE = 3;
static const unsigned int RGBA_PIXEL_SIZE = 4;
#ifdef ANDROID
static const int VGA_PIXEL_SIZE = RGBA_PIXEL_SIZE;
#else
static const int VGA_PIXEL_SIZE = RGB_PIXEL_SIZE;
#endif

class StreamConverter
{
    public:    
        ~StreamConverter();
        void InitStream(unsigned int width, unsigned int height, PreviewMode mode);
        bool Buffer2Image(Image* res, unsigned char* src_buffer, unsigned int src_buffer_size);

    private:
        PreviewMode _mode;
        Image _attr; // including _attr->buffer
};
}// namespace Capture
} // namespace RealSenseID