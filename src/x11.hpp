// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

#include "image.hpp"

#include <string>
#include <functional>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

/**
 * @class x11
 * @brief X11 window to draw the image.
 */
class x11 {
public:
    /**
     * @brief Callback for key press event.
     *
     * @param[in] key code
     *
     * @return false if event loop must bu aborted
     */
    using key_handler_fn = std::function<bool(KeySym)>;

    ~x11();

    /**
     * @brief Create X11 window.
     *
     * @param[in] border space between parent and this window
     *
     * @throw std::runtime_error in case of errors
     */
    void create(size_t border);

    /**
     * @brief Set window title.
     *
     * @param[in] title new parent's window title
     */
    void set_title(const char* title) const;

    /**
     * @brief Set new image for drawing.
     *
     * @param[in] img image to load
     * @param[in] x initial X coordinate of image on window (top-left corner)
     * @param[in] y initial Y coordinate of image on window (top-left corner)
     */
    void set_image(const image& img, ssize_t x, ssize_t y);

    /**
     * @brief Move image to the new position.
     *
     * @param[in] x new X coordinate of image on window (top-left corner)
     * @param[in] y new Y coordinate of image on window (top-left corner)
     */
    void move_image(ssize_t x, ssize_t y);

    /**
     * @brief Run event loop.
     *
     * @param[in] cb callback for key press events
     * @param[in] exit_unfocus exit loop if the window has lost input focus
     */
    void run(key_handler_fn cb, bool exit_unfocus) const;

    /** @brief Get width of the window. */
    inline size_t width() const { return width_; }
    /** @brief Get height of the window. */
    inline size_t height() const { return height_; }

    /** @brief Get X coordinate of image on window (top-left corner). */
    inline ssize_t img_x() const { return img_x_; }
    /** @brief Get Y coordinate of image on window (top-left corner). */
    inline ssize_t img_y() const { return img_y_; }
    /** @brief Get width of the image. */
    inline size_t img_w() const { return image_ ? image_->width : 0; }
    /** @brief Get height of the image. */
    inline size_t img_h() const { return image_ ? image_->height : 0; }

private:
    /**
     * @brief Send expose event to redraw the window.
     */
    void redraw() const;
    int getXresourceColor(const char* color) const;

private:
    /** @brief X11 display. */
    Display* display_ = nullptr;
    /** @brief Our X11 window. */
    Window wnd_ = 0;
    /** @brief Parent X11 window. */
    Window parent_ = 0;
    /** @brief X11 graphics context. */
    GC gc_ = 0;
    /** @brief Width of the window. */
    size_t width_ = 0;
    /** @brief Height of the window. */
    size_t height_ = 0;
    /** @brief Color depth. */
    size_t depth_ = 0;

    /** @brief X11 image descriptor. */
    XImage* image_ = nullptr;
    /** @brief X coordinate of image on window (top-left corner). */
    ssize_t img_x_ = 0;
    /** @brief Y coordinate of image on window (top-left corner). */
    ssize_t img_y_ = 0;

    /** @brief Original title of parent window. */
    std::string parent_title_;
};
