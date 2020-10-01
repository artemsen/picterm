// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "viewer.hpp"
#include "image_ldr.hpp"
#include "x11.hpp"

/** @brief Minimum scale (1%). */
constexpr size_t scale_min = 1;
/** @brief Minimum scale (1000%). */
constexpr size_t scale_max = 1000;
/** @brief Scale step used on zoom in/out. */
constexpr size_t scale_step = 5;
/** @brief Move step used on positioning. */
constexpr size_t move_step = 10;

void viewer::show()
{
    img_ = load_image(file_name);
    wnd_.create(border);

    if (!scale) {
        calc_scale(scale_op::optimal);
    }
    refresh();

    wnd_.run([this](KeySym key) { return this->on_keypress(key); }, exit_unfocus);
}

void viewer::refresh()
{
    // prepare image to show
    image img;
    if (scale != 100) {
        img = img_.resize(scale);
    } else {
        img = img_; // todo
    }
    if (img.transparent) {
        img = img.add_grid();
    }

    // recalculate position of image on window
    wnd_.updateWindowAttributes(border);
    ssize_t img_x;
    ssize_t img_y;
    const size_t wnd_w = wnd_.width();
    const size_t wnd_h = wnd_.height();

    if (img.width < wnd_w) {
        // width size fits into window
        img_x = wnd_w / 2 - img.width / 2;
    } else {
        // move to save center of previous image
        const ssize_t delta = static_cast<ssize_t>(wnd_.img_w()) - img.width;
        img_x = wnd_.img_x() + delta / 2;
        if (img_x > 0) {
            img_x = 0;
        } else if (img_x + img.width < wnd_w) {
            img_x = wnd_w - img.width;
        }
    }
    if (img.height < wnd_h) {
        // height size fits into window
        img_y = wnd_h / 2 - img.height / 2;
    } else {
        // move to save center of previous image
        const ssize_t delta = static_cast<ssize_t>(wnd_.img_h()) - img.height;
        img_y = wnd_.img_y() + delta / 2;
        if (img_y > 0) {
            img_y = 0;
        } else if (img_y + img.height < wnd_h) {
            img_y = wnd_h - img.height;
        }
    }

    wnd_.set_image(img, img_x, img_y);

    std::string title = file_name;
    title += " [";
    title += std::to_string(img_.width);
    title += 'x';
    title += std::to_string(img_.height);
    title += ' ';
    title += std::to_string(scale);
    title += "%]";
    wnd_.set_title(title.c_str());
}

bool viewer::calc_scale(scale_op op)
{
    const size_t old_scale = scale;

    switch (op) {
    case scale_op::zoom_in:
        if (scale < scale_max) {
            scale += scale_step;
            if (scale > scale_max) {
                scale = scale_max;
            }
        }
        break;

    case scale_op::zoom_out:
        if (scale != scale_min) {
            if (scale - scale_min > scale_step) {
                scale -= scale_step;
            } else {
                scale = scale_min;
            }
        }
        break;

    case scale_op::optimal:
        // 100% or less to fit the window
        wnd_.updateWindowAttributes(border);
        scale = 100;
        if (wnd_.width() < img_.width) {
            scale = 100 * (1.0f / (static_cast<float>(img_.width) / wnd_.width()));
        }
        if (wnd_.height() < img_.height) {
            const size_t min_scale = 100 * (1.0f / (static_cast<float>(img_.height) / wnd_.height()));
            if (min_scale < scale) {
                scale = min_scale;
            }
        }
        break;
    }

    return scale != old_scale;
}

void viewer::change_scale(scale_op op)
{
    if (calc_scale(op)) {
        refresh();
    }
}

void viewer::change_scale(size_t sc)
{
    if (scale != sc) {
        scale = sc;
        refresh();
    }
}

void viewer::change_position(move_op mv)
{
    wnd_.updateWindowAttributes(border);
    const size_t wnd_w = wnd_.width();
    const size_t wnd_h = wnd_.height();
    const size_t img_w = wnd_.img_w();
    const size_t img_h = wnd_.img_h();
    ssize_t img_x = wnd_.img_x();
    ssize_t img_y = wnd_.img_y();

    if (img_x > 0 && img_x + img_w < wnd_w && img_y > 0 && img_y + img_h < wnd_h) {
        return; // the whole image inside the window
    }

    switch (mv) {
    case move_op::left:
        if (img_x <= 0) {
            img_x += move_step;
            if (img_x > 0) {
                img_x = 0;
            }
        }
        break;
    case move_op::right:
        if (img_x + img_w >= wnd_w) {
            img_x -= move_step;
            if (img_x + img_w < wnd_w) {
                img_x = wnd_w - img_w;
            }
        }
        break;
    case move_op::up:
        if (img_y <= 0) {
            img_y += move_step;
            if (img_y > 0) {
                img_y = 0;
            }
        }
        break;
    case move_op::down:
        if (img_y + img_h >= wnd_h) {
            img_y -= move_step;
            if (img_y + img_h < wnd_h) {
                img_y = wnd_h - img_h;
            }
        }
        break;
    }

    if (img_x != wnd_.img_x() || img_y != wnd_.img_y()) {
        wnd_.move_image(img_x, img_y);
    }
}

bool viewer::on_keypress(KeySym key)
{
    // clang-format off
    switch (key) {
        case XK_Left:
        case XK_KP_Left:
        case XK_h:
            change_position(move_op::left);
            break;
        case XK_Right:
        case XK_KP_Right:
        case XK_l:
            change_position(move_op::right);
            break;
        case XK_Up:
        case XK_KP_Up:
        case XK_k:
            change_position(move_op::up);
            break;
        case XK_Down:
        case XK_KP_Down:
        case XK_j:
            change_position(move_op::down);
            break;

        case XK_plus:
        case XK_equal:
        case XK_KP_Add:
            change_scale(scale_op::zoom_in);
            break;
        case XK_minus:
        case XK_KP_Subtract:
            change_scale(scale_op::zoom_out);
            break;
        case XK_BackSpace:
            change_scale(scale_op::optimal);
            break;

        case XK_1: change_scale(10); break;
        case XK_2: change_scale(20); break;
        case XK_3: change_scale(30); break;
        case XK_4: change_scale(40); break;
        case XK_5: change_scale(50); break;
        case XK_6: change_scale(60); break;
        case XK_7: change_scale(70); break;
        case XK_8: change_scale(80); break;
        case XK_9: change_scale(90); break;
        case XK_0: change_scale(100); break;

        case XK_Escape:
        case XK_Cancel:
        case XK_Return:
        case XK_KP_Enter:
        case XK_F3:
        case XK_F4:
        case XK_F10:
        case XK_q:
        case XK_e:
        case XK_x:
            return false;

        default:
            break;
    }
    // clang-format off
    return true;
}
