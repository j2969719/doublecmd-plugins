wlximagemagick
========
Image viewer plugin. Supported formats: dds, tga, pcx, bmp, webp.

![plug-preview](https://i.imgur.com/Hb0lAHc.png)

## Notes
Requires `gtkimageview` library. If you have ImageMagick 6 (for example, Debian/Ubuntu-based distributions) then use `#include <wand/MagickWand.h>` instead `#include <MagickWand/MagickWand.h>`.

## Dependencies
![arch](https://wiki.archlinux.org/favicon.ico) `pacman -S gtkimageview imagemagick`
