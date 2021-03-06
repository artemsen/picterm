# Picterm: preview images in terminal window

![CI](https://github.com/artemsen/picterm/workflows/CI/badge.svg)

Creates a new X11 window as a child of the currently focused one (e.g. terminal
window), loads the image from the specified file, and draws the image inside the
new window.

![Screenshot](https://user-images.githubusercontent.com/1169976/85752455-1ce1a400-b714-11ea-80f6-4ecfc4dbccc0.png)

## Supported image formats

- JPEG (via libjpeg);
- PNG (via libpng);
- GIF (via giflib, without animation).

## Key bindings

- `Arrows` and vim-like moving keys (`hjkl`): Move view point;
- `+`, `=`: Zoom in;
- `-`: Zoom out;
- `Backspace`: Set optimal scale: 100% or fit to window;
- `1`, `2`, ..., `0`: Set scale to 10%, 20%, ..., 100%;
- `Esc`, `Enter`, `F10`, `q`, `e`, `x`: Exit the program.

## Build and install

```
./configure
make
make install
```

Arch users can install the program via [AUR](https://aur.archlinux.org/packages/picterm).
