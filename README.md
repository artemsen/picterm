# Picterm: preview images in terminal window

![CI](https://github.com/artemsen/picterm/workflows/CI/badge.svg)

Creates a new X11 window as a child of the currently focused one (e.g. terminal
window), loads the image from the specified file, and draws the image inside the
new window.

## Supported image formats

- JPEG (via libjpeg);
- PNG (via libpng);
- GIF (via libgif, without animation).
