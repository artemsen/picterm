// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "x11.hpp"

#include <cstring>
#include <stdexcept>

#include <X11/Xatom.h>
#include <X11/Xresource.h>

int getXresourceColor(const char* color, Display* display_)
{
    XrmInitialize();
    char* resource_manager = XResourceManagerString(display_);
    if (!resource_manager) {
        return -1;
    }

    XrmDatabase db = XrmGetStringDatabase(resource_manager);
    if (!db) {
        return -1;
    }

    XrmValue value;
    char *type;
    if (XrmGetResource(db, color, "", &type, &value)) {
        return strtol(value.addr + 1, NULL, 16); // first char is '#'
    } else {
        return -1;
    }
}

x11::~x11()
{
    if (!parent_title_.empty()) {
        set_title(parent_title_.c_str());
    }
    if (gc_) {
        XFreeGC(display_, gc_);
    }
    if (image_) {
        XDestroyImage(image_);
    }
    if (wnd_) {
        XUnmapWindow(display_, wnd_);
        XDestroyWindow(display_, wnd_);
    }
    if (display_) {
        XCloseDisplay(display_);
    }
}

void x11::create(size_t border)
{
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        throw std::runtime_error("Unable to open X11 display");
    }

    // get currently focused window to use it as parent
    const char* windowId = getenv("WINDOWID");
    if (windowId && *windowId) {
        parent_ = atoi(windowId);
    } else {
        int revert;
        XGetInputFocus(display_, &parent_, &revert);
    }
    if (!parent_) {
        throw std::runtime_error("Parent window not found, try to set WINDOWID");
    }

    // save current parent's title to restore them on exit
    XTextProperty prop;
    XGetWMName(display_, parent_, &prop);
    if (prop.nitems > 0) {
        int count = 0;
        char** list = nullptr;
        Xutf8TextPropertyToTextList(display_, &prop, &list, &count);
        if (count) {
            parent_title_ = list[0];
            XFreeStringList(list);
            XFree(prop.value);
        }
    }

    XWindowAttributes attr;
    XGetWindowAttributes(display_, parent_, &attr);
    width_ = attr.width - border * 2;
    height_ = attr.height - border * 2;
    depth_ = attr.depth;

    int colorbg = getXresourceColor("picterm.background", display_);
    if (colorbg == -1) {
        colorbg = 0;
    }
    // create overlay window
    wnd_ = XCreateSimpleWindow(display_, parent_, border, border, width_, height_, 0, 0, colorbg);

    const int screen = DefaultScreen(display_);
    gc_ = XCreateGC(display_, wnd_, 0, nullptr);
    XSetForeground(display_, gc_, WhitePixel(display_, screen));
    XSetBackground(display_, gc_, BlackPixel(display_, screen));

    XMapWindow(display_, wnd_);
    XSetInputFocus(display_, wnd_, RevertToParent, CurrentTime);
}

void x11::set_title(const char* title) const
{
    char* text = const_cast<char*>(title);
    unsigned char* prop = reinterpret_cast<unsigned char*>(text);
    XChangeProperty(display_, parent_,
        XInternAtom(display_, "_NET_WM_NAME", False),
        XA_STRING, 8, PropModeReplace, prop, strlen(title));
}

void x11::set_image(const image& img, ssize_t x, ssize_t y)
{
    // XCreateImage takes ownership of the image buffer and free it with XDestroyImage
    const size_t img_data_sz = img.height * img.width * sizeof(image::rgba_t);
    char* img_data = static_cast<char*>(malloc(img_data_sz));
    if (!img_data) {
        throw std::bad_alloc();
    }
    memcpy(img_data, img.data.data(), img_data_sz);

    // get currently filled area to determine whether we need to clear the window
    const size_t filled_x1 = img_x_ > 0 ? img_x_ : 0;
    const size_t filled_x2 = img_x_ + img_w() > width_ ? width_ : img_x_ + img_w();
    const size_t filled_y1 = img_y_ > 0 ? img_y_ : 0;
    const size_t filled_y2 = img_y_ + img_h() > height_ ? height_ : img_y_ + img_h();

    // Recreate the X image
    if (image_) {
        XDestroyImage(image_);
    }
    image_ = XCreateImage(display_,
        DefaultVisual(display_, DefaultScreen(display_)),
        depth_, ZPixmap, 0, img_data,
        img.width, img.height, sizeof(image::rgba_t) * 8, 0);

    // clear the window if new image doesn't cover the old one
    const size_t cover_x1 = x > 0 ? x : 0;
    const size_t cover_x2 = x + img_w() > width_ ? width_ : x + img_w();
    const size_t cover_y1 = y > 0 ? y : 0;
    const size_t cover_y2 = y + img_h() > height_ ? height_ : y + img_h();
    if (cover_x1 > filled_x1 || cover_x2 < filled_x2 || cover_y1 > filled_y1 || cover_y2 < filled_y2) {
        XClearWindow(display_, wnd_);
    }

    move_image(x, y);
}

void x11::move_image(ssize_t x, ssize_t y)
{
    img_x_ = x;
    img_y_ = y;
    redraw();
}

void x11::run(key_handler_fn cb, bool exit_unfocus) const
{
    XPutImage(display_, wnd_, gc_, image_, 0, 0, img_x_, img_y_, image_->width, image_->height);

    XEvent event;
    XSelectInput(display_, wnd_, ExposureMask | KeyPressMask | FocusChangeMask);
    while (1) {
        XNextEvent(display_, &event);
        if (event.type == Expose && event.xexpose.count == 0) {
            XPutImage(display_, wnd_, gc_, image_, 0, 0, img_x_, img_y_, image_->width, image_->height);
        } else if (event.type == KeyPress) {
            const KeySym key = XLookupKeysym(&event.xkey, 0);
            if (!cb(key)) {
                break;
            }
        } else if (event.type == FocusOut && exit_unfocus) {
            break;
        }
    }
}

void x11::redraw() const
{
    XEvent expose;
    memset(&expose, 0, sizeof(expose));
    expose.type = Expose;
    expose.xexpose.window = wnd_;
    XSendEvent(display_, wnd_, False, ExposureMask, &expose);
}
