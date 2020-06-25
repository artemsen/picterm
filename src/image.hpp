// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

#include <cstdint>
#include <vector>

/**
 * @class image
 * @brief Image container (RGBA, 32 bits per pixel, 8 bits per color).
 */
class image
{
  public:
    using rgba_t = uint32_t;

    /**
     * @brief Resize image.
     *
     * @param[in] percent scale factor
     *
     * @return transformed image instance
     */
    image resize(size_t percent) const;

    /**
     * @brief Add grid as a background for transparent image.
     *
     * @param[in] step grid step (size of a single cell)
     * @param[in] clr grid color
     *
     * @return transformed image instance
     */
    image add_grid(size_t step = 10, rgba_t clr = 0x404040) const;

    /** @brief Image data array. */
    std::vector<rgba_t> data;
    /** @brief Width of the image. */
    size_t width = 0;
    /** @brief Height of the image. */
    size_t height = 0;
    /** @brief Flag indicated that the image has valid alpha channel. */
    bool transparent = false;
};
