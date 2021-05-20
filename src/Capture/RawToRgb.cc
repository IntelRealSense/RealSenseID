// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Preview.h"
#include <cstdint>

namespace RealSenseID
{
namespace Capture
{
inline uint8_t Raw8Value(const uint8_t* src, int baseoff, int mod5)
{
    return (mod5 >= 4) ? 0 : *(src + baseoff);
}

void RotatedRaw2Rgb(const Image& src_img, Image& dst_img)
{
    constexpr int rgb_pixel_size = 3;
    unsigned int src_height = src_img.height, src_width = src_img.width; // width height is the same
    unsigned int dst_height = src_width, dst_width = src_height;         // rotating image

    unsigned int line = src_img.size / src_height;

    const unsigned char* src = src_img.buffer;
    unsigned char* dst = dst_img.buffer;

    const int palette = 128;

    unsigned int src_x = 0, src_y = 0;
    unsigned int dst_x = 0, dst_y = dst_height - 1;
    unsigned int i;

    for (i = 0; i < src_img.size; i++)
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

        unsigned int cur_rgb_pixel_offset = rgb_pixel_size * (dst_y * dst_width + dst_x);
        *(dst + cur_rgb_pixel_offset) = br1;
        *(dst + cur_rgb_pixel_offset + 1) = g;
        *(dst + cur_rgb_pixel_offset + 2) = br0;

        dst_y--;
        src_x++;

        /* Move to the next line*/
        if (src_x == src_width)
        {
            src_x = 0;
            src_y++;
            dst_y = dst_height - 1;
            dst_x++;
        }
    }

    // change image attr to match the convertion
    dst_img.height = dst_height;
    dst_img.width = dst_width;
    dst_img.stride = dst_img.size / dst_img.height;
}
} // namespace Capture
} // namespace RealSenseID