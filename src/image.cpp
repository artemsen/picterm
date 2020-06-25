// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "image.hpp"

image image::resize(size_t percent) const
{
    const float scale = static_cast<float>(percent) / 100.0;

    image img;
    img.width = scale * width;
    img.height = scale * height;
    img.transparent = transparent;
    img.data.resize(img.width * img.height);

    for (size_t y = 0; y < img.height; ++y) {
        const size_t row_src = static_cast<size_t>(static_cast<float>(y) / scale) * width;
        const size_t row_dst = y * img.width;
        for (size_t x = 0; x < img.width; ++x) {
            img.data[row_dst + x] = data[row_src + static_cast<size_t>(static_cast<float>(x) / scale)];
        }
    }

    return img;
}

image image::add_grid(size_t step /*= 10*/, rgba_t clr /*= 0x404040*/) const
{
    image img = *this;
    const rgba_t clr2 = clr - 0x00101010;

    for (size_t y = 0; y < img.height; ++y) {
        for (size_t x = 0; x < img.width; ++x) {
            rgba_t* pixel = &img.data[x + y * width];

            const uint8_t alpha = *pixel >> 24;
            if (alpha != 0xff) {
                const rgba_t bkg = (x / step) % 2 != (y / step) % 2 ? clr : clr2;
                const rgba_t dst = *pixel;
                const uint32_t ra = 255 - alpha;
                // clang-format off
                *pixel = (((((bkg >> 0)  & 0xff) * ra +
                            ((dst >> 0)  & 0xff) * alpha) >> 8)) |
                         (((((bkg >> 8)  & 0xff) * ra +
                            ((dst >> 8)  & 0xff) * alpha)     )  & ~0xff) |
                         (((((bkg >> 16) & 0xff) * ra +
                            ((dst >> 16) & 0xff) * alpha) << 8)  & ~0xffff) |
                         (((((bkg >> 24) & 0xff) * ra +
                            ((dst >> 24) & 0xff) * alpha) << 16) & ~0xffffff);
                // clang-format on
            }
        }
    }
    return img;
}
