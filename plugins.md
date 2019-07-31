Plugins
=======

- [WCX plugins](#wcx)

- WDX plugins: [binary (compiled) plugins](#wdx-bin) and [Lua scripts](#wdx-lua)

- [WFX plugins](#wfx)

- [WLX plugins](#wlx) (Note: Currently all listed below WLX plugins works with GTK2-version of DC only.)

---
<a name="wcx"><h3>WCX plugins</h3></a>

- [libarchive_crap](plugins/wcx/libarchive_crap)<br>
Simple WCX plugin (view and unpack only), based on `libarchive` ([supported formats](https://github.com/libarchive/libarchive#supported-formats)).


---
<a name="wdx-bin"><h3>WDX: Binary (compiled) plugins</h3></a>

- [calcsize](plugins/wdx/calcsize)<br>
Folder search by size.

- [crappcre](plugins/wdx/crappcre)<br>
Search file contents using regular expressions and extract search results. It can also work with file names.

- [datetimestr](plugins/wdx/datetimestr)<br>
Getting the time the file was last modified, last accessed or last changed. The time format is fully customizable (see `settings.ini`).

- [desktop_entry](plugins/wdx/desktop_entry)<br>
Reads data from the .desktop shortcut sections for hints and search.

- [emblems](plugins/wdx/emblems)<br>
Getting files/folders emblems.

- [emptydir](plugins/wdx/emptydir)<br>
Plugin for checking whether directory is empty. Can be used for highlighting or searching.

- [gdescription](plugins/wdx/gdescription)<br>
Getting file's content type and human readable description of the content type, detection audio, video and image files (by content type).

- [gimgsize](plugins/wdx/gimgsize)<br>
Getting image format, format description, size, width and height.

- [gitrepo](plugins/wdx/gitrepo)<br>
Getting some information from local copy of Git repository.

- [gunixmounts](plugins/wdx/gunixmounts)<br>
Mount points: name, FS type and other.

- [poppler_info](plugins/wdx/poppler_info)<br>
Basic information about PDF.

- [simplechecksum](plugins/wdx/simplechecksum)<br>
Plugin is able to calculate and display hash values for any file. Supported algorithms: CRC32, MD5, SHA1, SHA256 and SHA512. Can be used to search files by hash value.

- [simplefileinfo](plugins/wdx/simplefileinfo)<br>
A small wdx-plugin for Double Commander which provides additional information about the file via libmagic and stat.


---
<a name="wdx-lua"><h3>WDX: Lua scripts</h3></a>
Note: Some scripts uses command line utilities.

- [7zipwdx.lua](plugins/wdx/scripts/7zipwdx.lua)<br>
Getting some information (method, files and folder count, etc) from archives (using 7-Zip). You can see list of fields in `fields` (first column).

- [apkinfowdx.lua](plugins/wdx/scripts/apkinfowdx.lua)<br>
Getting some information from APK files. Fields: Label, Version, Install Location, Uses Permissions, Locales, Supports Screens, Native Code.

- [archivecommentwdx.lua](plugins/wdx/scripts/archivecommentwdx.lua)<br>
Reading comment from ZIP and RAR files.

- [bintstwdx.lua](plugins/wdx/scripts/bintstwdx.lua)<br>
Checking: binary or Unicode text file (encoding UTF-8, UTF-16 or UTF-32 and byte order, detection by BOM).

- [caseduplwdx.lua](plugins/wdx/scripts/caseduplwdx.lua)<br>
Search for duplicates with the same name but with a different case.

- [crapxmlwdx.lua](plugins/wdx/scripts/crapxmlwdx.lua)<br>
Getting some information from FB2 files ([more complete alternative](https://github.com/doublecmd/plugins/tree/master/wdx/fb2wdx)).

- [descriptionwdx.lua](plugins/wdx/scripts/descriptionwdx.lua)<br>
Script reads file descriptions from `descript.ion`.

- [dfwdx.lua](plugins/wdx/scripts/dfwdx.lua)<br>
Mount points: name, FS type, all/used/available size and other.

- [diffwdx.lua](plugins/wdx/scripts/diffwdx.lua)<br>
Search in diff file with reference file or diff from Git or SVN repository.

- [duwdx.lua](plugins/wdx/scripts/duwdx.lua)<br>
Returns directory size or files on a file system.

- [elfheaderinfo.lua](plugins/wdx/scripts/elfheaderinfo.lua)<br>
Getting some ELF header information.<br>
*cross platform*

- [encawdx.lua](plugins/wdx/scripts/encawdx.lua)<br>
Detection of file encoding. Requires `enca`.

- [exiftoolwdx.lua](plugins/wdx/scripts/exiftoolwdx.lua)<br>
Getting information with [ExifTool](https://www.sno.phy.queensu.ca/~phil/exiftool/). You can see list of fields in `fields` (first column).

- [extstatwdx.lua](plugins/wdx/scripts/extstatwdx.lua)<br>
Getting file count and total size of them. List of file extensions (edit manually): table `ext_allowed` in the beginning of script.

- [fatattrwdx.lua](plugins/wdx/scripts/fatattrwdx.lua)<br>
Getting MS-DOS attributes in a FAT file system. Requires `fatattr`.

- [fb2isvalidwdx](plugins/wdx/scripts/fb2isvalidwdx)<br>
Validating FictionBook2 (FB2) files (first test can be used with ANY XML-based files).

- [filenamechrstatwdx.lua](plugins/wdx/scripts/filenamechrstatwdx.lua)<br>
Returns count of characters in file name: total, alphanumeric, spaces, letters (all, lower case or upper case letters), digits and punctuation characters. You can choose the full path, path, file name or file extension.<br>
*cross platform*

- [filenamematchwdx.lua](plugins/wdx/scripts/filenamematchwdx.lua)<br>
Returns part of file name for sorting (getting title, volume, chapter, etc).<br>
*cross platform*

- [filenamereplacewdx.lua](plugins/wdx/scripts/filenamereplacewdx.lua)<br>
Returns new file name after replacing total or part of file name by the list (see and edit `RplFiles` in the beginning of script).<br>
*cross platform*

- [fixfilenamewdx.lua](plugins/wdx/scripts/fixfilenamewdx.lua)<br>
Fixing file name encoding with `iconv`.

- [getfaclwdx.lua](plugins/wdx/scripts/getfaclwdx.lua)<br>
Returns file permissions (ACL). Requires `getfacl`.

- [getfattrwdx.lua](plugins/wdx/scripts/getfattrwdx.lua)<br>
Getting MS-DOS attributes in a FAT file system. Requires `getfattr`.

- [gitinfowdx.lua](plugins/wdx/scripts/gitinfowdx.lua)<br>
Getting some information from local copy of Git repository. Requires `git`.

- [hexdecheaderwdx.lua](plugins/wdx/scripts/hexdecheaderwdx.lua) and [hexheaderwdx.lua](plugins/wdx/scripts/hexheaderwdx.lua)<br>
Examples: search by hex value. Now Double Commander can do it itself.<br>
*cross platform*

- [infilelistwdx.lua](plugins/wdx/scripts/infilelistwdx.lua)<br>
Search file name in custom list of file names.

- [libextractorwdx.lua](plugins/wdx/scripts/libextractorwdx.lua)<br>
Returns metadata from files. You can see list of fields in `fields` (first column). Requires `extract`.

- [pkginfowdx.lua](plugins/wdx/scripts/pkginfowdx.lua)<br>
Returns some information from Arch Linux packages.

- [quasiexpanderwdx.lua](plugins/wdx/scripts/quasiexpanderwdx.lua)<br>
Returns part of file name (by delimiter).
*cross platform*

- [randomcharswdx.lua](plugins/wdx/scripts/randomcharswdx.lua)<br>
Returns random alphanumeric character(s) (case-sensitive), by default up to 10.<br>
*cross platform*

- [ru2enutf8wdx.lua](plugins/wdx/scripts/ru2enutf8wdx.lua)<br>
Converts keyboard layout Ru <> En. Requires `luautf8` module.<br>
*cross platform*

- [ru2enwdx.lua](plugins/wdx/scripts/ru2enwdx.lua)<br>
Converts keyboard layout Ru <> En.<br>
*cross platform*

- [scrtiptfileinfowdx.lua](plugins/wdx/scripts/scrtiptfileinfowdx.lua)<br>
Returns various information about file using command line utilities. Uses [fileinfo.sh](scripts/fileinfo.sh).

- [selinuxfilelabelswdx.lua](plugins/wdx/scripts/selinuxfilelabelswdx.lua)<br>
Getting SELinux file labels.

- [someaudioext4findfileswdx.lua](plugins/wdx/scripts/someaudioext4findfileswdx.lua)<br>
Checking if file(s) exists in folder by extension. List of file extensions (edit manually): table `extlist` in the beginning of script.<br>
For "Find files" dialog.<br>
*cross platform*

- [somefilesindirwdx.lua](plugins/wdx/scripts/somefilesindirwdx.lua)<br>
Like script above but for columns set.<br>
*cross platform*

- [sqlitetxtsearchwdx.lua](plugins/wdx/scripts/sqlitetxtsearchwdx.lua)<br>
Plugin-example: getting text from SQLite3 base. Requires `lsqlite3` module.<br>
*cross platform*

- [stringmatchwdx.lua](plugins/wdx/scripts/stringmatchwdx.lua)<br>
Plugin-example: search text in files, fields will create with patterns.


- [svninfowdx.lua](plugins/wdx/scripts/svninfowdx.lua)<br>
Getting some information from local copy of SVN repository. Requires `svn`.

- [tarfilescountwdx.lua](plugins/wdx/scripts/tarfilescountwdx.lua) + [tarfilescountwdx.lng](plugins/wdx/scripts/tarfilescountwdx.lng)<br>
Returns count of files, folders and symlinks it TAR files (list of file extension by default: tar, gz, bz2, lzma, xz, tgz, tbz, tbz2, tzma, tzx, tlz).

- [textsearchwdx.lua](plugins/wdx/scripts/textsearchwdx.lua) + [textsearchwdx.lng](plugins/wdx/scripts/textsearchwdx.lng)<br>
Search text in files. List of file extensions and used converters: table `commands` in the beginning of script.

- [transmissionwdx.lua](plugins/wdx/scripts/transmissionwdx.lua)<br>
Returns information from TORRENT files. Requires `transmission-show`.

- [trashwdx.lua](plugins/wdx/scripts/trashwdx.lua)<br>
Script for using in trash folder: getting original file name and deletion date.


---
<a name="wfx"><h3>WFX plugins</h3></a>

- [adbplugin-x64](plugins/wfx/adbplugin-x64)<br>
PKGBUILD file for [Android ADB](http://www.uniqtec.eu/applications/android-adb.html).

- [clipboard](plugins/wfx/clipboard)<br>
View clipboard content.

- [cmdoutput](plugins/wfx/cmdoutput)<br>
View output of command line utilities (see `settings.ini`).

- [envlist](plugins/wfx/envlist)<br>
View list of environment variables.

- [filelist](plugins/wfx/filelist)<br>
Temporary panel, virtual folder that allows keeping links to frequently used files.

- [gtkrecent](plugins/wfx/gtkrecent)<br>
View list of recently used files (also support: open, view or delete any entry).

- [tmppanel_crap](plugins/wfx/tmppanel_crap)<br>
Temporary panel, see FileList description.


---
<a name="wlx"><h3>WLX plugins</h3></a>

- [abiword-gtk2](plugins/wlx/abiword-gtk2)<br>
Displays DOC, RTF, DOT, ABW, AWT, ZABW. Requires AbiWord (GTK2 version).

- [atril-gtk2](plugins/wlx/atril-gtk2)<br>
Displays PDF, DjVu, TIFF, PostScipt, CBR, CBZ, XPS. Requires Atril 2 (GTK2 version).

- [crapmagick](plugins/wlx/crapmagick)<br>
Displays TTF, OTF, PCX.
If you have ImageMagick 6 (for example, Debian/Ubuntu-based distributions) then use `#include <wand/MagickWand.h>` instead `#include <MagickWand/MagickWand.h>`.

- [dcpoppler](plugins/wlx/dcpoppler)<br>
PDF Viewer.

- [evince2](plugins/wlx/evince2)<br>
Displays PDF, DjVu, TIFF, PostScipt, CBR. Requires Evince 2 (GTK2 version).

- [fileinfo](plugins/wlx/fileinfo)<br>
Displays various information about file using command line utilities.

- [gtk3test](plugins/wlx/gtk3test)<br>
Attempts to use GTK3 widgets library, follow link.

- [gtk_socket](plugins/wlx/gtk_socket)<br>
Wrapper (embedding with socket help) for different scripts and utilities, follow link.

- [gtkimgview](plugins/wlx/gtkimgview)<br>
Image viewer plugin. Supported formats (all image formats supported by GdkPixbuf, the list can be differ on your system): xpm, xbm, tif, tiff, targa, tga, svg.gz, svgz, svg, qif, qtif, ppm, pgm, pbm, pnm, png, jpg, jpe, jpeg, jpf, j2k, jpx, jpc, jp2, cur, ico, icns, gif, bmp, ani.

- [gtksourceview](plugins/wlx/gtksourceview)<br>
Displays source code files with syntax highlighting. Requires `gtksourceview-2.0`.

- [hx_webkit_crap](plugins/wlx/hx_webkit_crap)<br>
Document viewer based on WebKitGTK2 and using HTML Export from Oracle Outside In Technology.

- [imagemagick](plugins/wlx/imagemagick)<br>
Image viewer plugin. Supported formats: dds, tga, pcx, bmp, webp.
If you have ImageMagick 6 (for example, Debian/Ubuntu-based distributions) then use `#include <wand/MagickWand.h>` instead `#include <MagickWand/MagickWand.h>`.

- [libarchive_crap](plugins/wlx/libarchive_crap)<br>
Displays file content with `libarchive` ([supported formats](https://github.com/libarchive/libarchive#supported-formats)).

- [mimescript](plugins/wlx/mimescript)<br>
Displays various information about file using command line utilities. Detection by MIME type.

- [mpv](plugins/wlx/mpv)<br>
Media player plugin. Requires `mpv`.

- [nfoview](plugins/wlx/nfoview)<br>
Displays NFO, DIZ.

- [scrolledimg](plugins/wlx/scrolledimg)<br>
Displays GIF.

- [vte_in_quickview](plugins/wlx/vte_in_quickview)<br>
Embeds Virtual Terminal Emulator (VTE) widget (GTK2 version).

- [vte_ncdu](plugins/wlx/vte_ncdu)<br>
Embeds Virtual Terminal Emulator (VTE) widget (GTK2 version) and runs `ncdu`.

- [wlxdisplay](plugins/wlx/wlxdisplay)<br>
Displays TTF, OTF, PCX. Requires `display` (ImageMagick tool).

- [wlxgvim](plugins/wlx/wlxgvim)<br>
Displays file content with GVim.

- [wlxsurf](plugins/wlx/wlxsurf)<br>
Displays Web-pages (*.htm, *.html, *.xhtml) with Surf web browser.

- [wlxvimb](plugins/wlx/wlxvimb)<br>
Displays Web-pages (*.htm, *.html, *.xhtml) with Vimb web browser.

- [wlxwebkit](plugins/wlx/wlxwebkit)<br>
The given plug-in allows to look through Web-pages (*.htm, *.html). It is based on WebKitGTK2 engine.

- [wlxwebkit_crap](plugins/wlx/wlxwebkit_crap)<br>
Like `wlxwebkit` but for any files if you can convert it to HTML file (see `settings.ini`).

- [yet_another_vte_plugin](plugins/wlx/yet_another_vte_plugin)<br>
Embeds Virtual Terminal Emulator (VTE) widget (GTK2 version) and runs command line utilities (see `settings.ini`).<br>
More usable than `vte_in_quickview` or `vte_ncdu`.

- [zathura](plugins/wlx/zathura)<br>
Displays PDF, DjVu, PostScipt, CBR. Requires `zathura`.
