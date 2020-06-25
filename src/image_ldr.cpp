// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>


#include "image_ldr.hpp"

#include <array>
#include <memory>
#include <cstring>
#include <system_error>

/**
 * @struct loader
 * @brief Image loader description.
 */
struct loader {
    using file_header_t = std::array<uint8_t, 16>;

    /** @brief Format description. */
    const char* desc;

    /**
     * @brief Function used for checking if file format is supported.
     *
     * @param[in] header file header data
     *
     * @return true if file format is supported
     */
    bool (*check)(const file_header_t& header);

    /**
     * @brief Function used for loading image from file.
     *
     * @param[in] file file to load
     *
     * @throw std::runtime_error if decoder fails
     *
     * @return image instance
     */
    image (*load)(FILE* fd);
};

////////////////////////////////////////////////////////////////////////////////
// JPEG image support
////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_LIBJPEG
#include <jpeglib.h>

static bool jpg_check(const loader::file_header_t& header)
{
    static const uint8_t sig[] = { 0xff, 0xd8 };
    return memcmp(header.data(), sig, sizeof(sig)) == 0;
}

static image jpg_load(FILE* file)
{
    image img;

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

    return img;
}
#endif // HAVE_LIBJPEG

////////////////////////////////////////////////////////////////////////////////
// PNG image support
////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_LIBPNG
#include <png.h>

static bool png_check(const loader::file_header_t& header)
{
    static const uint8_t sig[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    return memcmp(header.data(), sig, sizeof(sig)) == 0;
}

static image png_load(FILE* file)
{
    image img;
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
        img.transparent = !(color_type == PNG_COLOR_TYPE_RGB ||
                            color_type == PNG_COLOR_TYPE_GRAY ||
                            color_type == PNG_COLOR_TYPE_PALETTE);
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

    return img;
}
#endif // HAVE_LIBPNG

////////////////////////////////////////////////////////////////////////////////
// GIF image support
////////////////////////////////////////////////////////////////////////////////
#ifdef HAVE_LIBGIF
#include <gif_lib.h>

static bool gif_check(const loader::file_header_t& header)
{
    static const uint8_t sig[] = { 'G', 'I', 'F' };
    return memcmp(header.data(), sig, sizeof(sig)) == 0;
}

static int gif_reader(GifFileType* gft, GifByteType* buffer, int sz)
{
    return fread(buffer, 1, sz, reinterpret_cast<FILE*>(gft->UserData));
}

static image gif_load(FILE* file)
{
    image img;

    int err = 0;
    std::unique_ptr<GifFileType, void(*)(GifFileType*)> gif_ptr(
        DGifOpen(file, gif_reader, &err),
        [](GifFileType* gif){ DGifCloseFile(gif, nullptr); });
    if (!gif_ptr.get()) {
        throw std::runtime_error(GifErrorString(err));
    }
    GifFileType* gif = gif_ptr.get();

    // decode with high-level API
    if (DGifSlurp(gif) != GIF_OK) {
        throw std::runtime_error(GifErrorString(gif->Error));
    }
    if (!gif->SavedImages) {
        throw std::runtime_error("Something wrong with giflib");
    }

    img.width = gif->SWidth;
    img.height = gif->SHeight;
    img.transparent = true;
    img.data.resize(img.height * img.width);

    // We don't support animation, will show the first frame only
    const GifImageDesc& frame_desc = gif->SavedImages->ImageDesc;
    const int frame_x = frame_desc.Left;
    const int frame_y = frame_desc.Top;
    const int frame_w = frame_desc.Width;
    const int frame_h = frame_desc.Height;
    const GifColorType* clr_map = gif->SColorMap ? gif->SColorMap->Colors : frame_desc.ColorMap->Colors;

    for (int y = 0; y < frame_h; ++y) {
        image::rgba_t* pixel = &img.data[frame_y * img.width + y * img.width + frame_x]; 
        const uint8_t* raster = &gif->SavedImages->RasterBits[y * img.width];
        for (int x = 0; x < frame_w; ++x) {
            const uint8_t& color = raster[x];
            if (color == gif->SBackGroundColor) {
                *pixel = 0;
            } else {
                const GifColorType& rgb = clr_map[color];
                *pixel = 0xff000000 | rgb.Red << 16 | rgb.Green << 8 | rgb.Blue;
            }
            ++pixel;
        }
    }

    return img;
}
#endif // HAVE_LIBGIF

/** @brief List of image loader handlers. */
static const loader loaders[] = {
    { "JPEG (libjpeg)",
#ifdef HAVE_LIBJPEG
        jpg_check, jpg_load
#else
        nullptr, nullptr
#endif // HAVE_LIBJPEG
    },
    { "PNG (libpng)",
#ifdef HAVE_LIBPNG
        png_check, png_load
#else
        nullptr, nullptr
#endif // HAVE_LIBPNG
    },
    { "GIF (libgif)",
#ifdef HAVE_LIBGIF
        gif_check, gif_load
#else
        nullptr, nullptr
#endif // HAVE_LIBGIF
    },
};

image load_image(const char* file)
{
    std::unique_ptr<FILE, int (*)(FILE*)> fd(fopen(file, "rb"), fclose);
    if (!fd) {
        throw std::system_error(errno, std::system_category());
    }

    loader::file_header_t header;
    if (fread(header.data(), 1, header.size(), fd.get()) != header.size()) {
        throw std::system_error(errno, std::system_category());
    }
    rewind(fd.get());

    for (auto& it : loaders) {
        if (it.check && it.check(header)) {
            return it.load(fd.get());
        }
    }

    throw std::runtime_error("Unsupported format");
}

void print_formats()
{
    for (auto& it : loaders) {
        printf("  %-15s: %s\n", it.desc, it.check ? "YES" : "NO");
    }
}
