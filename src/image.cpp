// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "image.hpp"

#include <cstring>
#include <memory>
#include <system_error>
#include <vector>

#ifdef HAVE_LIBPNG
#include <png.h>

/**
 * @brief Check if file is in PNG format.
 *
 * @param[in] file file to check
 *
 * @return true if file is PNG
 */
static bool is_png(FILE* file)
{
    static const uint8_t sig[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    uint8_t hdr[sizeof(sig)];
    return fread(hdr, 1, sizeof(hdr), file) == sizeof(hdr) && memcmp(hdr, sig, sizeof(sig)) == 0;
}

/**
 * @brief Load PNG file.
 *
 * @param[in] file file to load
 *
 * @throw std::runtime_error if decoder (libpng) fails
 */
static void load_png(FILE* file, image& img)
{
    png_structp png = nullptr;
    png_infop info = nullptr;

    try {
        png = png_create_read_struct(
            PNG_LIBPNG_VER_STRING, nullptr,
            [](png_structp, png_const_charp err) {
                throw std::runtime_error(err);
            },
            [](png_structp, png_const_charp) {});

        if (!png) {
            throw std::runtime_error("Unable to create libpng read object");
        }
        info = png_create_info_struct(png);
        if (!info) {
            throw std::runtime_error("Unable to create libpng info object");
        }

        png_init_io(png, file);
        png_read_info(png, info);
        png_set_bgr(png);

        img.width = png_get_image_width(png, info);
        img.height = png_get_image_height(png, info);

        const png_byte color_type = png_get_color_type(png, info);
        const png_byte bit_depth = png_get_bit_depth(png, info);

        // Read any color_type into 8bit depth, RGBA format.
        if (bit_depth == 16) {
            png_set_strip_16(png);
        }
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png);
        }
        // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
            png_set_expand_gray_1_2_4_to_8(png);
        }
        if (png_get_valid(png, info, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png);
        }
        // These color_type don't have an alpha channel then fill it with 0xff.
        img.transparent = !(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE);
        if (!img.transparent) {
            png_set_filler(png, 0xff, PNG_FILLER_AFTER);
        }
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png);
        }

        png_read_update_info(png, info);

        img.data.resize(img.height * img.width);
        std::vector<png_bytep> rows(img.height);
        for (size_t y = 0; y < img.height; ++y) {
            rows[y] = reinterpret_cast<png_bytep>(&img.data[y * img.width]);
        }
        png_read_image(png, rows.data());

        png_destroy_read_struct(&png, &info, nullptr);

    } catch (const std::exception&) {
        if (png) {
            png_destroy_read_struct(&png, info ? &info : nullptr, nullptr);
        }
        throw;
    }
}
#endif // HAVE_LIBPNG

#ifdef HAVE_LIBJPEG
#include <jpeglib.h>

/**
 * @brief Check if file is in JPEG format.
 *
 * @param[in] file file to check
 *
 * @return true if file is JPEG
 */
static bool is_jpg(FILE* file)
{
    static const uint8_t sig[] = { 0xff, 0xd8 };
    uint8_t hdr[sizeof(sig)];
    return fread(hdr, 1, sizeof(hdr), file) == sizeof(hdr) && memcmp(hdr, sig, sizeof(sig)) == 0;
}

/**
 * @brief Load JPG file.
 *
 * @param[in] file file to load
 *
 * @throw std::runtime_error if decoder (libjpeg) fails
 */
static void load_jpg(FILE* file, image& img)
{
    std::unique_ptr<jpeg_decompress_struct, void (*)(jpeg_decompress_struct*)> jpg_ptr(
        new jpeg_decompress_struct,
        jpeg_destroy_decompress);

    jpeg_decompress_struct* jpg = jpg_ptr.get();
    jpeg_error_mgr err;

    jpg->err = jpeg_std_error(&err);

    err.emit_message = [](j_common_ptr, int) {};
    err.error_exit = [](j_common_ptr jpg) {
        char jpg_msg[JMSG_LENGTH_MAX];
        (*(jpg->err->format_message))(jpg, jpg_msg);
        throw std::runtime_error(jpg_msg);
    };

    jpeg_create_decompress(jpg);
    jpeg_stdio_src(jpg, file);
    jpeg_read_header(jpg, TRUE);
    jpeg_start_decompress(jpg);

    img.width = jpg->output_width;
    img.height = jpg->output_height;

    img.data.resize(img.height * img.width);
    const size_t row_sz = jpg->num_components * img.width;
    std::vector<uint8_t> buffer(row_sz);
    uint8_t* ptr = buffer.data();
    while (jpg->output_scanline < jpg->output_height) {
        image::rgba_t* pixel = &img.data[jpg->output_scanline * img.width];
        jpeg_read_scanlines(jpg, &ptr, 1);
        for (size_t i = 0; i < row_sz; i += jpg->num_components) {
            // not the most optimal way, but...
            switch (jpg->num_components) {
                case 1:
                    *pixel = 0xff000000 | buffer[i] | buffer[i] << 8 | buffer[i] << 16;
                    break;
                case 3:
                    *pixel = 0xff000000 | buffer[i + 2] | buffer[i + 1] << 8 | buffer[i] << 16;
                    break;
                case 4:
                    *pixel = buffer[i + 3] << 24 | buffer[i + 2] | buffer[i + 1] << 8 | buffer[i] << 16;
                    break;
                default:
                    throw std::runtime_error(std::to_string(jpg->num_components) + " components not supported yet");
            }
            ++pixel;
        }
    }

    jpeg_finish_decompress(jpg);
}
#endif // HAVE_LIBJPEG

image::image(const char* file)
{
    std::unique_ptr<FILE, int (*)(FILE*)> fd(fopen(file, "rb"), fclose);
    if (!fd) {
        throw std::system_error(errno, std::system_category());
    }

#ifdef HAVE_LIBJPEG
    const bool fmt_jpg = is_jpg(fd.get());
    rewind(fd.get());
    if (fmt_jpg) {
        load_jpg(fd.get(), *this);
        return;
    }
#endif // HAVE_LIBJPEG

#ifdef HAVE_LIBPNG
    const bool fmt_png = is_png(fd.get());
    rewind(fd.get());
    if (fmt_png) {
        load_png(fd.get(), *this);
        return;
    }
#endif // HAVE_LIBPNG

    throw std::runtime_error("Unsupported format");
}

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
