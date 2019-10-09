hx_webkit_qt_crap
========
Document viewer based on Qt5WebKit and using HTML Export from Oracle Outside In Technology.

![plug-preview](https://i.imgur.com/64QDFBO.png)

## Compatibility
- `doublecmd-qt5`

## Notes
- download [HTML Export](https://www.oracle.com/technetwork/middleware/content-management/downloads/oit-dl-otn-097435.html) (**hx-***x-x-x***-linux-x86-64.zip**)
- extract archive and copy all files from the `redist` folder to an empty folder here.
- copy `exsimple` and `default.cfg` from `/sdk/demo` to the `redist` folder here
- optionally, you can also copy /sdk/`template` folder somewhere (you must specify an absolute path in `default.cfg`)

## Dependencies
![arch](https://wiki.archlinux.org/favicon.ico) `pacman -S qt5-webkit`
