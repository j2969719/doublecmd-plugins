Build plugins in Debian/Ubuntu
------------------------------

Development packages and note (if it is needed) for each plugin.<br>
Note: There are several versions of Ubuntu distributions and derivatives, so when specifying versions, we will only consider LTS releases.

List of plugins with a brief description can be found here [plugins.md](plugins.md).

First of all, you need a compiler (`gcc` or `g++`) and the `make` program, an easy way is to install the `build-essential` package.
Now install required packages, open plugin directory, launch terminal and use the following commands:


To begin, you will need a compiler (`gcc` or `g++`) and the `make` program, an easy way is to install the `build-essential` package. Next, install required packages, open plugin directory, launch terminal and use the following commands:

```sh
cd src
make
```

You can pack the plugin files into a `.tar` archive: use `make dist` after `make`.
See the description of the `cm_AddPlugin` command [here](https://doublecmd.github.io/doc/en/cmds.html#cm_AddPlugin) (DC >= 1.1.0).

Alternatively, you can use the scripts in the [plugins](plugins) directory:

- `make_all.sh`: allows to compile all possible plugins, i.e. `make`, but for all plugins. Don't forget to install all required packages for the necessary (or all) plugins. Also see the note about the `bit7z` library below.<br>
- `make_all_clean.sh`: cleanup after compilation, i.e. `make clean`, but for all plugins.<br>
- `pack_to_tarballs.sh`: pack all plugins into separate archives,  i.e. `make dist`, but for all plugins.<br>
- `run_ldd.sh`: generates a list of required libraries (`plugins/dist/.broken.log`, for all plugins) that are not installed on your system. See [FAQ: This is not a valid plugin](https://doublecmd.github.io/doc/en/faq.html#not_valid).

Note:<br>
If you are not a programmer or if you rarely build programs and libraries from source, then installing a large number of development packages can be inconvenient for several reasons: a good solution might be to use the same version of your operation system in a virtual machine (QEMU, VirtualBox and so on) or a minimal installation of Debian/Ubuntu in chroot (schroot, sbuild, pbuilder)


---
### Plugins:

- [WCX plugins](#wcx)

- [WDX plugins](#wdx)

- [WFX plugins](#wfx)

- WLX plugins: [GTK2/GTK3](#wlxgtk), [Qt5/Qt6](#wlxqt)

- [DSX plugins](#dsx)


---
<a name="wcx"><h3>WCX plugins</h3></a>

- [bit7z](plugins/wcx/bit7z)<br>
`apt install build-essential libglib2.0-dev cmake ninja-build git ca-certificates`<br>
The last four packages for the `bit7z` library, see the `src/third_party` subfolder.

- [cmdconv_crap](plugins/wcx/cmdconv_crap)<br>
`apt install build-essential libglib2.0-dev`

- [cmdoutput](plugins/wcx/cmdoutput)<br>
`apt install build-essential libglib2.0-dev`

- [fb2bin_crap](plugins/wcx/fb2bin_crap)<br>
`apt install build-essential libglib2.0-dev libxml2-dev`

- [gcrypt_hash_crap](plugins/wcx/gcrypt_hash_crap)<br>
`apt install build-essential libglib2.0-dev libgcrypt20-dev libgpg-error-dev`

- [hexstr_crap](plugins/wcx/hexstr_crap)<br>
`apt install build-essential libglib2.0-dev`

- [imagemagick_ico_crap](plugins/wcx/imagemagick_ico_crap)<br>
`apt install build-essential libglib2.0-dev libmagickwand-6.q16-dev`<br>
`Makefile` is universal for ImageMagick 6 and ImageMagick 7, Debian/Ubuntu and derivative distributions still use ImageMagick 6, so use `make 6` instead of `make`.

- [imagemagick_gif_crap](plugins/wcx/imagemagick_gif_crap)<br>
`apt install build-essential libglib2.0-dev libmagickwand-6.q16-dev`<br>
`Makefile` is universal for ImageMagick 6 and ImageMagick 7, Debian/Ubuntu and derivative distributions still use ImageMagick 6, so use `make 6` instead of `make`.

- [libarchive_crap](plugins/wcx/libarchive_crap)<br>
`apt install build-essential libglib2.0-dev libarchive-dev`

- [linkfiles_crap](plugins/wcx/linkfiles_crap)<br>
`apt install build-essential libglib2.0-dev`

- [m3u8_crap](plugins/wcx/m3u8_crap)<br>
`apt install build-essential libglib2.0-dev libtagc0-dev`

- [mozlz4json_crap](plugins/wcx/mozlz4json_crap)<br>
`apt install build-essential liblz4-dev`


---
<a name="wdx"><h3>WDX plugins</h3></a>

- [bit7z](plugins/wdx/bit7z)<br>
`apt install build-essential`<br>
Requires the `bit7z` library, see [bit7z packer plugin](#wcx).

- [calcsize](plugins/wdx/calcsize)<br>
`apt install build-essential`

- [crx_crap](plugins/wdx/crx_crap)<br>
`apt install build-essential libarchive-dev libjson-glib-dev`

- [datetimestr](plugins/wdx/datetimestr)<br>
`apt install build-essential libglib2.0-dev`

- [desktop_entry](plugins/wdx/desktop_entry)<br>
`apt install build-essential libglib2.0-dev`

- [emblems](plugins/wdx/emblems)<br>
`apt install build-essential libglib2.0-dev`

- [emptydir](plugins/wdx/emptydir)<br>
`apt install build-essential`

- [fewfiles](plugins/wdx/fewfiles)<br>
`apt install build-essential`

- [gdescription](plugins/wdx/gdescription)<br>
`apt install build-essential libglib2.0-dev`

- [gfileinfo](plugins/wdx/gfileinfo)<br>
`apt install build-essential libglib2.0-dev`

- [gimgsize](plugins/wdx/gimgsize)<br>
Debian 11.0+/Ubuntu 22.04+: `apt install build-essential libglib2.0-dev libgdk-pixbuf-2.0-dev`<br>
Previous versions: `apt install build-essential libglib2.0-dev libgdk-pixbuf2.0-dev`

- [gitrepo](plugins/wdx/gitrepo)<br>
`apt install build-essential pkg-config libgit2-dev`

- [gunixmounts](plugins/wdx/gunixmounts)<br>
`apt install build-essential libglib2.0-dev`

- [libarchive_crap](plugins/wdx/libarchive_crap)<br>
`apt install build-essential pkg-config libarchive-dev`

- [poppler_info](plugins/wdx/poppler_info)<br>
`apt install build-essential libpoppler-glib-dev`

- [simplechecksum](plugins/wdx/simplechecksum)<br>
`apt install build-essential libgcrypt20-dev libgpg-error-dev`

- [simplefileinfo](plugins/wdx/simplefileinfo)<br>
`apt install build-essential libmagic-dev`

- [skipdotfiles](plugins/wdx/skipdotfiles)<br>
`apt install build-essential`

- [texttooltip](plugins/wdx/texttooltip)<br>
`apt install build-essential pkg-config libenca-dev libmagic-dev`


---
<a name="wfx"><h3>WFX plugins</h3></a>

- [aur_crap](plugins/wfx/aur_crap)<br>
`apt install build-essential`

- [avfs_crap](plugins/wfx/avfs_crap)<br>
`apt install build-essential pkg-config avfs`

- [clipboard](plugins/wfx/clipboard)<br>
`apt install build-essential libglib2.0-dev libgtk2.0-dev`

- [clipboard_files](plugins/wfx/clipboard_files)<br>
`apt install build-essential libglib2.0-dev libgtk2.0-dev`

- [cmdoutput](plugins/wfx/cmdoutput)<br>
`apt install build-essential libglib2.0-dev`

- [cmdoutput_panel](plugins/wfx/cmdoutput_panel)<br>
`apt install build-essential libglib2.0-dev`

- [contentfilter_crap](plugins/wfx/contentfilter_crap)<br>
`apt install build-essential libglib2.0-dev`

- [desktopfiles](plugins/wfx/desktopfiles)<br>
`apt install build-essential libglib2.0-dev`

- [envlist](plugins/wfx/envlist)<br>
`apt install build-essential libglib2.0-dev`

- [fnmatch_crap](plugins/wfx/fnmatch_crap)<br>
`apt install build-essential libglib2.0-dev libmagic-dev`

- [gtkrecent](plugins/wfx/gtkrecent)<br>
`apt install build-essential libgtk2.0-dev`

- [gvfs_quickmount](plugins/wfx/gvfs_quickmount)<br>
`apt install build-essential libglib2.0-dev`

- [href_crap](plugins/wfx/href_crap)<br>
`apt install build-essential libglib2.0-dev libcurl4-gnutls-dev libxml2-dev`

- [icasecopy](plugins/wfx/icasecopy)<br>
`apt install build-essential`

- [null_crap](plugins/wfx/null_crap)<br>
`apt install build-essential`

- [physfs_crap](plugins/wfx/physfs_crap)<br>
`apt install build-essential libphysfs-dev`

- [proclst](plugins/wfx/proclst)<br>
`apt install build-essential`

- [taglib_crap](plugins/wfx/taglib_crap)<br>
`apt install build-essential libglib2.0-dev libtagc0-dev`

- [tmppanel_crap](plugins/wfx/tmppanel_crap)<br>
`apt install build-essential libglib2.0-dev`

- [trash_crap](plugins/wfx/trash_crap)<br>
`apt install build-essential libglib2.0-dev`

- [udisk_loopdev_crap](plugins/wfx/udisk_loopdev_crap)<br>
`apt install build-essential libglib2.0-dev libudisks2-dev`

- [wfx_scripts](plugins/wfx/wfx_scripts)<br>
`apt install build-essential libglib2.0-dev`<br>


---
<a name="wlxgtk"><h3>WLX plugins: GTK2/GTK3</h3></a>
Note: GTK2 plugins require DC GTK2 version, GTK3 plugins require DC GTK3 version

- [abiword-gtk2](plugins/wlx/abiword-gtk2)<br>
GTK2: `apt install build-essential libabiword-dev`<br>
Requires AbiWord (GTK2 version).

- [abiword-gtk3](plugins/wlx/abiword-gtk3)<br>
GTK3: `apt install build-essential libabiword-dev`<br>
Requires AbiWord (GTK3 version).

- [atril-gtk2](plugins/wlx/atril-gtk2)<br>
GTK2: `apt install build-essential libgtk2.0-dev libatrilview-dev libatrildocument-dev`<br>
Requires Atril <= 1.16.1 (GTK2 version).

- [csvview_gtk2](plugins/wlx/csvview_gtk2)<br>
GTK2: `apt install build-essential libgtk2.0-dev libenca-dev`

- [csvview_gtk3](plugins/wlx/csvview_gtk3)<br>
GTK3: `apt install build-essential libgtk-3-dev libenca-dev`

- [evince2](plugins/wlx/evince2)<br>
GTK2: `apt install build-essential libgtk2.0-dev libevview-dev libevdocument-dev`<br>
Requires Evince <= 2.32 (GTK2 version).

- [evince3](plugins/wlx/evince3)<br>
GTK3: `apt install build-essential libgtk-3-dev libevince-dev`<br>
Requires Evince (GTK3 version).

- [fileinfo](plugins/wlx/fileinfo)<br>
GTK2: : `apt install build-essential libgtk2.0-dev`<br>
GTK2/srcvw2: `apt install build-essential libgtk2.0-dev libgtksourceview2.0-dev`<br>
GTK3: `apt install build-essential libgtk-3-dev`

- [gstplayer](plugins/wlx/gstplayer)<br>
GTK2: `apt install build-essential libgtk2.0-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`

- [gtk_socket](plugins/wlx/gtk_socket)<br>
GTK2: `apt install build-essential libgtk2.0-dev libmagic-dev`<br>
GTK3: `apt install build-essential libgtk-3-dev libmagic-dev`<br>
Additions:<br>
src/abiword: `apt install build-essential libabiword-dev libjpeg-dev`<br>
src/evince: `apt install build-essential libevince-dev`<br>
src/libreoffice: `apt install build-essential libgtk-3-dev libreofficekit-dev`<br>
src/webkit2gtk: `apt install build-essential libwebkit2gtk-4.0-dev`<br>
src/yelp: `apt install build-essential libgtk-3-dev libwebkit2gtk-4.0-dev libyelp-dev`<br>
Note about `libwebkit2gtk-4.0-dev`: If this package is not found, then try `libwebkit2gtk-4.1-dev` instead.

- [gtk_socket_tst](plugins/wlx/gtk_socket_tst)<br>
GTK2: `apt install build-essential libglib2.0-dev libgtk2.0-dev libmagic-dev`<br>
GTK3: `apt install build-essential libglib2.0-dev libgtk-3-dev libmagic-dev`<br>
Additions:<br>
src/libreoffice: `apt install build-essential libgtk-3-dev libreofficekit-dev`<br>

- [gtkimgview](plugins/wlx/gtkimgview)<br>
GTK2: `apt install build-essential libgtkimageview-dev`

- [gtkimgview_crap](plugins/wlx/gtkimgview_crap)<br>
GTK2: `apt install build-essential libgtkimageview-dev`

- [gtkimgview_gtk3](plugins/wlx/gtkimgview_gtk3)<br>
GTK3: `apt install build-essential libgtk-3-dev`

- [gtkimgview_recumbent_gtk3](plugins/wlx/gtkimgview_recumbent_gtk3)<br> 
GTK3: `apt install build-essential libgtk-3-dev`

- [gtksourceview](plugins/wlx/gtksourceview)<br>
GTK2: `apt install build-essential libgtksourceview2.0-dev libenca-dev`

- [hx_webkit_crap](plugins/wlx/hx_webkit_crap)<br>
GTK2: `apt install build-essential libgtk2.0-dev libwebkitgtk-dev`

- [hx_webkit_gtk3_crap](plugins/wlx/hx_webkit_gtk3_crap)<br>
GTK3: `apt install build-essential libgtk-3-dev libwebkit2gtk-4.0-dev`
Note about `libwebkit2gtk-4.0-dev`: If this package is not found, then try `libwebkit2gtk-4.1-dev` instead.

- [imagemagick](plugins/wlx/imagemagick)<br>
GTK2:<br>
Debian 13.0+/Ubuntu 26.04+: `apt install build-essential libgtkimageview-dev libmagickwand-7.q16-dev`<br>
Previous versions: `apt install build-essential libgtkimageview-dev libmagickwand-6.q16-dev`<br>
`Makefile` is universal for ImageMagick 6 and ImageMagick 7: if you have ImageMagick 6, then use `make 6` instead of `make`.

- [jsonview_gtk2](plugins/wlx/jsonview_gtk2)<br>
GTK2: `apt install build-essential libgtk2.0-dev libjson-glib-dev`

- [jsonview_gtk3](plugins/wlx/jsonview_gtk3)<br>
GTK3: `apt install build-essential libgtk-3-dev libjson-glib-dev`

- [libarchive_crap](plugins/wlx/libarchive_crap)<br>
GTK2: `apt install build-essential libgtk2.0-dev libarchive-dev libenca-dev`<br>
GTK3: `apt install build-essential libgtk-3-dev libarchive-dev libenca-dev`

- [md4c_webkit](plugins/wlx/md4c_webkit)<br>
GTK2: `apt install build-essential libgtk2.0-dev libwebkitgtk-dev libmd4c-dev libmd4c-html0-dev`<br>
Requires md4c >= 0.4.4.

- [md4c_webkit_gtk3](plugins/wlx/md4c_webkit_gtk3)<br>
GTK3: `apt install build-essential libgtk-3-dev libwebkit2gtk-4.0-dev libmd4c-dev libmd4c-html0-dev`<br>
Note about `libwebkit2gtk-4.0-dev`: If this package is not found, then try `libwebkit2gtk-4.1-dev` instead.<br>
Requires md4c >= 0.4.4.

- [mimescript](plugins/wlx/mimescript)<br>
GTK2: `apt install build-essential libgtk2.0-dev libgtksourceview2.0-dev`

- [mpv](plugins/wlx/mpv)<br>
GTK2: `apt install build-essential libgtk2.0-dev`

- [mpv_alt](plugins/wlx/mpv_alt)<br>
GTK2: `apt install build-essential libgtk2.0-dev`<br>
GTK3: `apt install build-essential libgtk-3-dev`

- [nfoview](plugins/wlx/nfoview)<br>
GTK2: `apt install build-essential libgtk2.0-dev`

- [sqlview_gtk2](plugins/wlx/sqlview_gtk2)<br>
GTK2: `apt install build-essential libgtk2.0-dev libsqlite3-dev`

- [sqlview_gtk3](plugins/wlx/sqlview_gtk3)<br>
GTK3: `apt install build-essential libgtk-3-dev libsqlite3-dev`

- [symlinkerror](plugins/wlx/symlinkerror)<br>
GTK2: `apt install build-essential libgtk2.0-dev`

- [wlxpview](plugins/wlx/wlxpview)<br>
GTK2: `apt install build-essential libgtk2.0-dev libpoppler-glib-dev`<br>
GTK3: `apt install build-essential libgtk-3-dev libpoppler-glib-dev`

- [wlxwebkit](plugins/wlx/wlxwebkit)<br>
GTK2: `apt install build-essential libgtk2.0-dev libwebkitgtk-dev`

- [wlxwebkit_crap](plugins/wlx/wlxwebkit_crap)<br>
GTK2: `apt install build-essential libgtk2.0-dev libwebkitgtk-dev`

- [wlxwebkit_gtk3](plugins/wlx/wlxwebkit_gtk3)<br>
GTK3: `apt install build-essential libgtk-3-dev libwebkit2gtk-4.0-dev`
Note about `libwebkit2gtk-4.0-dev`: If this package is not found, then try `libwebkit2gtk-4.1-dev` instead.

- [yet_another_vte_plugin](plugins/wlx/yet_another_vte_plugin)<br>
GTK2: `apt install build-essential libgtk2.0-dev libvte-dev`

- [zathura](plugins/wlx/zathura)<br>
GTK2: `apt install build-essential libgtk2.0-dev`<br>
GTK3: `apt install build-essential libgtk-3-dev`<br>


---
<a name="wlxqt"><h3>WLX plugins: Qt5/Qt6</h3></a>
Note: Qt5 plugins require DC Qt5 version, Qt6 plugins require DC Qt6 version.<br>
You can use `make qt5` (Qt5 only), `make qt6` (Qt6 only) or `make` (both versions).

- [bit7z_qt_crap](plugins/wlx/bit7z_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`<br>
Requires the `bit7z` library, see [bit7z packer plugin](#wcx).

- [csvview_qt](plugins/wlx/csvview_qt)<br>
Qt5: `apt install build-essential pkg-config libglib2.0-dev libenca-dev qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config libglib2.0-dev libenca-dev qt6-base-dev`

- [dirchart_qml_qt_crap](plugins/wlx/dirchart_qml_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtdeclarative5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-declarative-dev libqt6opengl6-dev`<br>
Note about `libqt6opengl6-dev`: If this package is not found, then in your version of distribution this package has merged into the `qt6-base-dev` package, so just remove it from the list.

- [dirextchart_crap_qt](plugins/wlx/dirextchart_crap_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqt5charts5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-charts-dev libqt6opengl6-dev`<br>
Note about `libqt6opengl6-dev`: If this package is not found, then in your version of distribution this package has merged into the `qt6-base-dev` package, so just remove it from the list.<br>
Note about `qt6-charts-dev`: If this package is not found, then try `libqt6charts6-dev` instead.

- [dirsize_crap_qt](plugins/wlx/dirsize_crap_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqt5charts5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-charts-dev libqt6opengl6-dev`<br>
Note about `libqt6opengl6-dev`: If this package is not found, then in your version of distribution this package has merged into the `qt6-base-dev` package, so just remove it from the list.<br>
Note about `qt6-charts-dev`: If this package is not found, then try `libqt6charts6-dev` instead.

- [fileinfo_qt](plugins/wlx/fileinfo_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`

- [fontview_qt](plugins/wlx/fontview_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev libgl-dev`

- [htmlconv_qt_crap](plugins/wlx/htmlconv_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`

- [htmlview_qt_crap](plugins/wlx/htmlview_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`

- [hx_qt_crap](plugins/wlx/hx_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`

- [hx_webkit_qt_crap](plugins/wlx/hx_webkit_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqt5webkit5-dev`

- [imgview_crap_qml_qt](plugins/wlx/imgview_crap_qml_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtdeclarative5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-declarative-dev`

- [imgview_crap_qml_qt_crap](plugins/wlx/imgview_crap_qml_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtdeclarative5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-declarative-dev`

- [jsonview_qt](plugins/wlx/jsonview_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`

- [libarchive_qt_crap](plugins/wlx/libarchive_qt_crap)<br> 
Qt5: `apt install build-essential pkg-config qtbase5-dev libarchive-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev libarchive-dev`

- [md4c_qt](plugins/wlx/md4c_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libmd4c-dev libmd4c-html0-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev libmd4c-dev libmd4c-html0-dev`<br>
Requires md4c >= 0.4.4.

- [md4c_webkit_qt](plugins/wlx/md4c_webkit_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqt5webkit5-dev libmd4c-dev libmd4c-html0-dev`<br>
Requires md4c >= 0.4.4.

- [mpv_alt](plugins/wlx/mpv_alt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev libgl-dev`

- [pdf_crap_qml_qt](plugins/wlx/pdf_crap_qml_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtdeclarative5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-declarative-dev`

- [qmediaplayer_qt](plugins/wlx/qmediaplayer_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtmultimedia5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-multimedia-dev`

- [qtermwidget_qt_crap](plugins/wlx/qtermwidget_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqtermwidget5-0-dev libutf8proc-dev`

- [qtimgview](plugins/wlx/qtimgview)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-5compat-dev libgl-dev`

- [qtimgview_crap](plugins/wlx/qtimgview_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-5compat-dev libgl-dev`

- [qtpdfview_qt](plugins/wlx/qtpdfview_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtpdf5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-webengine-dev qt6-pdf-dev`

- [qtpdfview_qt_crap](plugins/wlx/qtpdfview_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtpdf5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-webengine-dev qt6-pdf-dev`

- [sqlview_qt](plugins/wlx/sqlview_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`

- [symlinkerror_qt](plugins/wlx/symlinkerror_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`

- [syntax-highlighting_qt](plugins/wlx/syntax-highlighting_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libkf5syntaxhighlighting-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev libkf6syntaxhighlighting-dev`

- [webengine_qt](plugins/wlx/webengine_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtwebengine5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-webengine-dev`

- [wlxwebkit_qt](plugins/wlx/wlxwebkit_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqt5webkit5-dev`

- [wlxwebkit_qt_crap](plugins/wlx/wlxwebkit_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqt5webkit5-dev`


---
<a name="dsx"><h3>DSX plugins</h3></a>

- [dsx_script](plugins/dsx/dsx_script)<br>
`apt install build-essential libglib2.0-dev`

- [git_ignored](plugins/dsx/git_ignored)<br>
`apt install build-essential`

- [git_modified](plugins/dsx/git_modified)<br>
`apt install build-essential`

- [git_untracked](plugins/dsx/git_untracked)<br>
`apt install build-essential`

- [gtkrecent](plugins/dsx/gtkrecent)<br>
`apt install build-essential libgtk2.0-dev`

- [locate_crap](plugins/dsx/locate_crap)<br>
`apt install build-essential libglib2.0-dev`

- [lslocks](plugins/dsx/lslocks)<br>
`apt install build-essential`

- [recollq_crap](plugins/dsx/recollq_crap)<br>
`apt install build-essential libglib2.0-dev`

- [tracker3_crap](plugins/dsx/tracker3_crap)<br>
`apt install build-essential libglib2.0-dev`

- [tracker_textsearch](plugins/dsx/tracker_textsearch)<br>
`apt install build-essential libtracker-sparql-3.0-dev`
