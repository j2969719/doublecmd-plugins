Build plugins in Debian/Ubuntu
------------------------------

Development packages and note (if it needed) for each plugin.

First of all, we need a compiler (`gcc` or `g++`) and the `make` program, so we will install the `build-essential` package.

- [WCX plugins](#wcx)

- [WDX plugins](#wdx)

- [WFX plugins](#wfx)

- WLX plugins: [GTK2](#wlxgtk) (for GTK2-version of DC only), [Qt5 or Qt6](#wlxqt) (for Qt-versions of DC only)

- [DSX plugins](#dsx)

List of plugins with a brief description on [one pages](plugins.md).
The [main page](README.md) of this repository.


---
<a name="wcx"><h3>WCX plugins</h3></a>

- [bit7z](plugins/wcx/bit7z)<br>
`apt install build-essential libglib2.0-dev cmake ninja-build git ca-certificates`<br>
The last four packages for the `bit7z` library, see the `src/third_party ` subfolder.

- [cmdconv_crap](plugins/wcx/cmdconv_crap)<br>
`apt install build-essential libglib2.0-dev`

- [cmdoutput](plugins/wcx/cmdoutput)<br>
`apt install build-essential libglib2.0-dev`

-[fb2bin_crap](plugins/wcx/fb2bin_crap)<br>
`apt install build-essential libglib2.0-dev libxml2-dev`

- [gcrypt_hash_crap](plugins/wcx/gcrypt_hash_crap)<br>
`apt install build-essential libglib2.0-dev libgcrypt20-dev libgpg-error-dev`

- [hexstr_crap](plugins/wcx/hexstr_crap)<br>
`apt install build-essential libglib2.0-dev`

- [imagemagick_ico_crap](plugins/wcx/imagemagick_ico_crap)<br>
`apt install build-essential libglib2.0-dev libmagickwand-6.q16-dev`<br>
`Makefile` is universal for ImageMagick 6 and ImageMagick 7, Debian/Ubuntu and derivative distributions still use ImageMagick 6, so use `make 6` instead of `make `.

- [imagemagick_gif_crap](plugins/wcx/imagemagick_gif_crap)<br>
`apt install build-essential libglib2.0-dev libmagickwand-6.q16-dev`<br>
`Makefile` is universal for ImageMagick 6 and ImageMagick 7, Debian/Ubuntu and derivative distributions still use ImageMagick 6, so use `make 6` instead of `make `.

- [libarchive_crap](plugins/wcx/libarchive_crap)<br>
`apt install build-essential libglib2.0-dev libarchive-dev`

- [linkfiles_crap](plugins/wcx/linkfiles_crap)<br>
`apt install build-essential libglib2.0-dev`

- [m3u8_crap](plugins/wcx/m3u8_crap)<br>
`apt install build-essential libglib2.0-dev libtagc0-dev`


---
<a name="wdx"><h3>WDX plugins</h3></a>

- [bit7z](plugins/wdx/bit7z)<br>
`apt install build-essential`<br>
Requires the `bit7z` library, see [bit7z packer plugin](#wcx).

- [calcsize](plugins/wdx/calcsize)<br>
`apt install build-essential`

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
`apt install build-essential libglib2.0-dev libgtk2.0-dev`

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
<a name="wlxgtk"><h3>WLX plugins: GTK2</h3></a>

- [abiword-gtk2](plugins/wlx/abiword-gtk2)<br>
`apt install build-essential libabiword-dev`<br>
Requires AbiWord (GTK2 version).

- [atril-gtk2](plugins/wlx/atril-gtk2)<br>
`apt install build-essential libgtk2.0-dev libatrilview-dev libatrildocument-dev`<br>
Requires GTK2 version, i.e. Atril <= 1.16.1.

- [csvview_gtk2](plugins/wlx/csvview_gtk2)<br>
`apt install build-essential libgtk2.0-dev libenca-dev`

- [dirsize_crap](plugins/wlx/dirsize_crap)<br>
`apt install build-essential libgtk2.0-dev`

- [evince2](plugins/wlx/evince2)<br>
`apt install build-essential libgtk2.0-dev libevview-dev libevdocument-dev`<br>
Requires GTK2 version, i.e. Evince <= 2.32.

- [fileinfo](plugins/wlx/fileinfo)<br>
`apt install build-essential libgtk2.0-dev libgtksourceview2.0-dev`

- [gstplayer](plugins/wlx/gstplayer)<br>
`apt install build-essential libgtk2.0-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`

- [gtk_socket](plugins/wlx/gtk_socket)<br>
`apt install build-essential libgtk2.0-dev libmagic-dev`<br>
Additions:`<br>
src/abiword: `apt install build-essential libabiword-dev`<br>
src/evince: `apt install build-essential libevince-dev`<br>
src/libreoffice: `apt install build-essential libgtk-3-dev libreofficekit-dev`<br>
src/webkit2gtk: `apt install build-essential libwebkit2gtk-4.0-dev`<br>
src/yelp: `apt install build-essential libgtk-3-dev libwebkit2gtk-4.0-dev libyelp-dev`

- [gtkimgview](plugins/wlx/gtkimgview)<br>
`apt install build-essential libgtkimageview-dev`

- [gtkimgview_crap](plugins/wlx/gtkimgview_crap)<br>
`apt install build-essential libgtkimageview-dev`

- [gtksourceview](plugins/wlx/gtksourceview)<br>
`apt install build-essential libgtksourceview2.0-dev libenca-dev`

- [jsonview_gtk2](plugins/wlx/jsonview_gtk2)<br>
`apt install build-essential libgtk2.0-dev libjson-glib-dev`

- [hx_webkit_crap](plugins/wlx/hx_webkit_crap)<br>
`apt install build-essential libgtk2.0-dev libwebkitgtk-dev`

- [imagemagick](plugins/wlx/imagemagick)<br>
`apt install build-essential libgtkimageview-dev libmagickwand-6.q16-dev`<br>
`Makefile` is universal for ImageMagick 6 and ImageMagick 7, Debian/Ubuntu and derivative distributions still use ImageMagick 6, so use `make 6` instead of `make `.

- [libarchive_crap](plugins/wlx/libarchive_crap)<br>
`apt install build-essential libgtk2.0-dev libarchive-dev libenca-dev`

- [md4c_webkit](plugins/wlx/md4c_webkit)<br>
`apt install build-essential libgtk2.0-dev libwebkitgtk-dev libmd4c-dev libmd4c-html0-dev`<br>
Requires md4c >= 0.4.4.

- [mimescript](plugins/wlx/mimescript)<br>
`apt install build-essential libgtk2.0-dev libgtksourceview2.0-dev`

- [mpv](plugins/wlx/mpv)<br>
`apt install build-essential libgtk2.0-dev`

- [nfoview](plugins/wlx/nfoview)<br>
`apt install build-essential libgtk2.0-dev`

- [sqlview_gtk2](plugins/wlx/sqlview_gtk2)<br>
`apt install build-essential libgtk2.0-dev libsqlite3-dev`

- [symlinkerror](plugins/wlx/symlinkerror)<br>
`apt install build-essential libgtk2.0-dev`

- [wlxpview](plugins/wlx/wlxpview)<br>
`apt install build-essential libgtk2.0-dev libpoppler-glib-dev`

- [wlxwebkit](plugins/wlx/wlxwebkit)<br>
`apt install build-essential libgtk2.0-dev libwebkitgtk-dev`

- [wlxwebkit_crap](plugins/wlx/wlxwebkit_crap)<br>
`apt install build-essential libgtk2.0-dev libwebkitgtk-dev`

- [yet_another_vte_plugin](plugins/wlx/yet_another_vte_plugin)<br>
`apt install build-essential libgtk2.0-dev libvte-dev`

- [zathura](plugins/wlx/zathura)<br>
`apt install build-essential libgtk2.0-dev`


---
<a name="wlxqt"><h3>WLX plugins: Qt5 or Qt6</h3></a>

We can use `make qt5` (Qt5 only), `make qt6` (Qt6 only) or `make` (both versions).

- [bit7z_qt_crap](plugins/wlx/bit7z_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`<br>
Requires the `bit7z` library, see [bit7z packer plugin](#wcx).

- [csvview_qt](plugins/wlx/csvview_qt)<br>
Qt5: `apt install build-essential pkg-config libglib2.0-dev libenca-dev qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config libglib2.0-dev libenca-dev qt6-base-dev`

- [dirchart_qml_qt_crap](plugins/wlx/dirchart_qml_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtdeclarative5-dev`

- [dirextchart_crap_qt](plugins/wlx/dirextchart_crap_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqt5charts5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-charts-dev libqt6opengl6-dev`<br>
Note about `libqt6opengl6-dev`: If this package is not found, then in your version of distribution this package has merged into the `qt6-base-dev` package, so just remove it from the list.

- [dirsize_crap_qt](plugins/wlx/dirsize_crap_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqt5charts5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev qt6-charts-dev libqt6opengl6-dev`<br>
Note about `libqt6opengl6-dev`: If this package is not found, then in your version of distribution this package has merged into the `qt6-base-dev` package, so just remove it from the list.

- [fileinfo_qt](plugins/wlx/fileinfo_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev`<br>
Qt6: `apt install build-essential pkg-config qt6-base-dev`

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
Qt5: `apt install build-essential pkg-config qtbase5-dev qtdeclarative5-dev`

- [imgview_crap_qml_qt_crap](plugins/wlx/imgview_crap_qml_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtdeclarative5-dev`

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

- [pdf_crap_qml_qt](plugins/wlx/pdf_crap_qml_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtdeclarative5-dev`

- [qmediaplayer_qt](plugins/wlx/qmediaplayer_qt)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev qtmultimedia5-dev`

- [qtermwidget_qt_crap](plugins/wlx/qtermwidget_qt_crap)<br>
Qt5: `apt install build-essential pkg-config qtbase5-dev libqtermwidget5-0-dev libutf8proc-dev`

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
Qt5: `apt install build-essential pkg-config qtbase5-dev libkf5syntaxhighlighting-dev`

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
