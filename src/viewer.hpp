// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

#include "image.hpp"
#include "x11.hpp"

#include <cstddef>

/**
 * @class viewer
 * @brief Image viewer.
 */
class viewer {
public:
    /**
     * @brief Show image.
     *
     * @throw std::exception in case of errors
     */
    void show();

private:
    /**
     * @brief Refresh image on the window.
     */
    void refresh();

    /** @brief Scale operation types. */
    enum class scale_op {
        zoom_in,
        zoom_out,
        optimal
    };

    /**
     * @brief Calculate new scale.
     *
     * @param[in] op requested operation type
     *
     * @return true if scale was changed
     */
    bool calc_scale(scale_op op);

    /**
     * @brief Change the scale.
     *
     * @param[in] op requested operation type
     */
    void change_scale(scale_op op);

    /**
     * @brief Change the scale.
     *
     * @param[in] scale scale to set
     */
    void change_scale(size_t sc);

    /** @brief Move operation types. */
    enum class move_op {
        left,
        right,
        up,
        down
    };

    /**
     * @brief Move view point.
     *
     * @param[in] op requested operation type
     */
    void change_position(move_op op);

    /**
     * @brief Key press handler (see x11::key_handler_fn).
     */
    bool on_keypress(KeySym key);

public:
    /** @brief Path to the file to show. */
    const char* file_name = nullptr;
    /** @brief Current image scale. */
    size_t scale = 0;
    /** @brief Window border size. */
    size_t border = 0;
    /** @brief Exit if window lost focus. */
    bool exit_unfocus = false;

private:
    /** @brief X11 window. */
    x11 wnd_;
    /** @brief Original image to show. */
    image img_;
};
