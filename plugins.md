Plugins
-------

- [WCX plugins](#wcx)

- WDX plugins: [binary (compiled) plugins](#wdx-bin) and [Lua scripts](#wdx-lua)

- [WFX plugins](#wfx)

- WLX plugins: [GTK2](#wlxgtk) (for GTK2-version of DC only), [Qt5](#wlxqt5) (for Qt5-version of DC only)

- [DSX plugins](#dsx)

---
<a name="wcx"><h3>WCX plugins</h3></a>

- [bit7z](plugins/wcx/bit7z)<br>
Packer plugin based on bit7z lib.

- [cmdconv_crap](plugins/wcx/cmdconv_crap)<br>
Converts multiple files via commandline utilities (Files -> Pack Files...).

- [cmdoutput](plugins/wcx/cmdoutput)<br>
View output of command line utilities on Ctrl+Page Down (see `settings_default.ini`).

- [fb2bin_crap](plugins/wcx/fb2bin_crap)<br>
Extract binary data from fb2.

- [gcrypt_hash_crap](plugins/wcx/gcrypt_hash_crap)<br>
Calculates a hash (Files -> Pack Files...), test file integrity (Files -> Test Archive(s)).

- [hexstr_crap](plugins/wcx/hexstr_crap)<br>
base16ish wtf.

- [imagemagick_gif_crap](plugins/wcx/imagemagick_gif_crap)<br>
Extract frames from GIF.

- [imagemagick_ico_crap](plugins/wcx/imagemagick_ico_crap)<br>
Extract pages from ICO.

- [libarchive_crap](plugins/wcx/libarchive_crap)<br>
A horrible attempt to make a simple WCX plugin based on `libarchive` ([supported formats](https://github.com/libarchive/libarchive#supported-formats) + [lizard](https://github.com/inikep/lizard)).

- [linkfiles_crap](plugins/wcx/linkfiles_crap)<br>
Creates hard or symbolic links for multiple files (Files -> Pack Files...).

- [m3u8_crap](plugins/wcx/m3u8_crap)<br>
Creates m3u8 files (Files -> Pack Files...).

- [mozlz4json_crap](plugins/wcx/mozlz4json_crap)<br>
View lz4 json from Mozilla.


---
<a name="wdx-bin"><h3>WDX: Binary (compiled) plugins</h3></a>

- [bit7z](plugins/wdx/bit7z)<br>
Getting some information from archives.

- [calcsize](plugins/wdx/calcsize)<br>
Folder search by size.

- ~[crx_crap](plugins/wdx/crx_crap)<br>
crx/xpi.~

- [datetimestr](plugins/wdx/datetimestr)<br>
Getting the time the file was last modified, last accessed or last changed. The time format is fully customizable (see `settings.ini`).

- [desktop_entry](plugins/wdx/desktop_entry)<br>
Reads data from the .desktop shortcut sections for hints and search.

- [emblems](plugins/wdx/emblems)<br>
Getting files/folders emblems.

- [emptydir](plugins/wdx/emptydir)<br>
Plugin for checking whether directory is empty. Can be used for highlighting or searching.

- [fewfiles](plugins/wdx/fewfiles)<br>
Shows a few files/folders located in some selected folder. Can be used to customize folder tooltips.

- [gdescription](plugins/wdx/gdescription)<br>
Getting file's content type and human readable description of the content type, detection audio, video and image files (by content type).

- ~[gfileinfo](plugins/wdx/gfileinfo)<br>
Getting GFile attributes~ (kinda useless and unfinished).

- [gimgsize](plugins/wdx/gimgsize)<br>
Getting image format, format description, size, width and height.

- [gitrepo](plugins/wdx/gitrepo)<br>
Getting some information from local copy of Git repository.

- [gunixmounts](plugins/wdx/gunixmounts)<br>
Mount points: name, FS type and other.

- [libarchive_crap](plugins/wdx/libarchive_crap)<br>
Getting some information from archives. Requires `libarchive`.

- [poppler_info](plugins/wdx/poppler_info)<br>
Basic information about PDF.

- [simplechecksum](plugins/wdx/simplechecksum)<br>
Plugin is able to calculate and display hash values for any file. Can be used to search files by hash value.

- [simplefileinfo](plugins/wdx/simplefileinfo)<br>
A small wdx-plugin for Double Commander which provides additional information about the file via libmagic and stat.

- [skipdotfiles](plugins/wdx/skipdotfiles)<br>
Excludes files/folders with a leading dot (hidden on unix-like) from search results.


---
<a name="wdx-lua"><h3>WDX: Lua scripts</h3></a>
Note: Some scripts uses command line utilities.

- [7zipwdx.lua](plugins/wdx/scripts/7zipwdx.lua)<br>
Getting some information (method, files and folder count, etc) from archives (using 7-Zip). You can see list of fields in `fields` (first column).

- [agewdx.lua](plugins/wdx/scripts/agewdx.lua) *(cross platform)*<br>
Returns age (the date of the last modification will be used), see description in the beginning of script.

- [apkinfowdx.lua](plugins/wdx/scripts/apkinfowdx.lua)<br>
Getting some information from APK files. Fields: Label, Version, Install Location, Uses Permissions, Locales, Supports Screens, Native Code.

- [appcrashwdx.lua](plugins/wdx/scripts/appcrashwdx.lua) *(cross platform)*<br>
Getting some information from CRASH-files on Debian/Ubuntu and Debian/Ubuntu-based distributions (/var/crash/*.crash). Only app crash files, not core dumps!

- [archivecommentwdx.lua](plugins/wdx/scripts/archivecommentwdx.lua)<br>
Reading comment from ZIP and RAR files.

- [attachedpicwdx.lua](plugins/wdx/scripts/attachedpicwdx.lua)<br>
Getting some information about attached pictures: MP3 (ID3v2.3 & ID3v2.4) and FLAC.

- [bintstwdx.lua](plugins/wdx/scripts/bintstwdx.lua)<br>
Checking: binary or Unicode text file (encoding UTF-8, UTF-16 or UTF-32 and byte order, detection by BOM).

- [caseduplwdx.lua](plugins/wdx/scripts/caseduplwdx.lua) *(cross platform)*<br>
Search for duplicates with the same name but with a different case.

- [changecasewdx.lua](plugins/wdx/scripts/changecasewdx.lua) *(cross platform)*<br>
Changing the case of letters, see description in the beginning of script.

- [checkfileextwdx.lua](plugins/wdx/scripts/checkfileextwdx.lua) *(cross platform)*<br>
Checking that the file extension matches the file type (by the file signatures) and returns some additional info. See details in the beginning of script.

- [checkfilenamewdx.lua](plugins/wdx/scripts/checkfilenamewdx.lua) *(cross platform)*<br>
Check filename limitations and recommendations, path lenght and other, see description in the beginning of script.

- [checkthumbswdx.lua](plugins/wdx/scripts/checkthumbswdx.lua) *(cross platform)*<br>
Getting some information from thumbnails files: created by Double Commander (Windows or Linux) or the system generator of thumbnails (Linux).

- [descriptionwdx.lua](plugins/wdx/scripts/descriptionwdx.lua)<br>
Script reads file descriptions from `descript.ion`.

- [djvuwdx.lua](plugins/wdx/scripts/djvuwdx.lua)<br>
Getting some information from DjVu files and searching text. Requires `djvused`.

- [elfheaderwdx.lua](plugins/wdx/scripts/elfheaderwdx.lua) *(cross platform)*<br>
Getting some ELF header information.

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

- [filecountwdx.lua](plugins/wdx/scripts/filecountwdx.lua) *(cross platform)*<br>
Returns file or directory count and directory size (without scanning symbolic links to folders).<br>
Only for columns set or tooltips!

- [filenamewdx.lua](plugins/wdx/scripts/filenamewdx.lua) *(cross platform)*<br>
Returns parts of full filename: Full name, Name, Base name, Extension, Path, Parent folder.

- [filenamechrstatwdx.lua](plugins/wdx/scripts/filenamechrstatwdx.lua) *(cross platform)*<br>
Returns count of characters in file name: total, alphanumeric, spaces, letters (all, lower case or upper case letters), digits and punctuation characters. You can choose the full path, path, file name or file extension.

- [filenamematchwdx.lua](plugins/wdx/scripts/filenamematchwdx.lua) *(cross platform)*<br>
Returns part of file name for sorting (getting title, volume, chapter, etc).

- [filenamereplacewdx.lua](plugins/wdx/scripts/filenamereplacewdx.lua) *(cross platform)*<br>
Returns new file name after replacing total or part of file name by the list (see and edit `RplFiles` in the beginning of script).

- [filenameunwdx.lua](plugins/wdx/scripts/filenameunwdx.lua) *(cross platform)*<br>
Returns the normalized (Unicode normalization) filename: NFC, NFD, NFKC, NFKD. Search for filenames with combined characters.<br>
Requires LuaJIT and [libunistring](https://www.gnu.org/software/libunistring/). See details in the beginning of script.

- [filetimeindirwdx.lua](plugins/wdx/scripts/filetimeindirwdx.lua) *(cross platform)*<br>
Returns date and time of oldest and newest files in directory. See details in the beginning of script.<br>
Only for columns set or tooltips!

- [filewinattrexwdx.lua](plugins/wdx/scripts/filewinattrexwdx.lua)<br>
Return file attributes (Windows only!). See details in the beginning of script.

- [folderbytextwdx.lua](plugins/wdx/scripts/folderbytextwdx.lua)<br>
Search for folders by contents of text files (without a recursive search, i.e. for file search tool only).

- [getfaclwdx.lua](plugins/wdx/scripts/getfaclwdx.lua)<br>
Returns file permissions (ACL). Requires `getfacl`.

- [getfattrwdx.lua](plugins/wdx/scripts/getfattrwdx.lua)<br>
Getting MS-DOS attributes in a FAT file system. Requires `getfattr`.

- [getfattrcustomwdx.lua](plugins/wdx/scripts/getfattrcustomwdx.lua)<br>
Getting custom extended attributes. Requires `getfattr`.

- [gitinfowdx.lua](plugins/wdx/scripts/gitinfowdx.lua)<br>
Getting some information from local copy of Git repository. Requires `git`.

- [groupswdx.lua](plugins/wdx/scripts/groupswdx.lua)<br>
Returns the group name by the file mask.

- [hexdecheaderwdx.lua](plugins/wdx/scripts/hexdecheaderwdx.lua) and [hexheaderwdx.lua](plugins/wdx/scripts/hexheaderwdx.lua) *(cross platform)*<br>
Examples: search by hex value. Now Double Commander can do it itself.

- [icalendarwdx.lua](plugins/wdx/scripts/icalendarwdx.lua) *(cross platform)*<br>
Getting some information from iCalendar files.

- [icowdx.lua](plugins/wdx/scripts/icowdx.lua) *(cross platform)*<br>
Getting some information from Windows icons (ICO images). See details in the beginning of script.

- [jpegwdx.lua](plugins/wdx/scripts/jpegwdx.lua) *(cross platform)*<br>
Getting some information from JPEG files. See details in the beginning of script.

- [libextractorwdx.lua](plugins/wdx/scripts/libextractorwdx.lua)<br>
Returns metadata from files. You can see list of fields in `fields` (first column). Requires `extract`.

- [lnkinfowdx.lua](plugins/wdx/scripts/lnkinfowdx.lua)<br>
Getting some information from Windows shell link (shortcut) file. Requires `lnkinfo`.

- [lslockswdx.lua](plugins/wdx/scripts/lslockswdx.lua)<br>
List local system locks.

- [marker](scripts/lua/marker) *(cross platform)*<br>
Marker for file highlighting like in *Colors* > *File types*, but "on the fly". See readme.md.

- [msgfulltextwdx.lua](plugins/wdx/scripts/msgfulltextwdx.lua) *(cross platform)*<br>
Returns full text of saved email messages. For Find files with plugins only! See details in the beginning of script.

- [msginfowdx.lua](plugins/wdx/scripts/msginfowdx.lua) *(cross platform)*<br>
Getting some information from headers of saved email messages. Supported formats: *.eml, *.msg and MH mailfile format (Sylpheed and other).

- [msofficeoldwdx.lua](plugins/wdx/scripts/msofficeoldwdx.lua)<br>
Getting some information from Microsoft Office 97-2003 files: *.doc, *.xls, *.ppt. Requires `gsf`.

- [pkginfowdx.lua](plugins/wdx/scripts/pkginfowdx.lua)<br>
Returns some information from Arch Linux packages.

- [pngwdx.lua](plugins/wdx/scripts/pngwdx.lua) *(cross platform)*<br>
Getting some information from PNG files.

- [poheaderwdx.lua](plugins/wdx/scripts/poheaderwdx.lua) *(cross platform)*<br>
Getting some information from PO-files (gettext).

- [psdwdx.lua](plugins/wdx/scripts/psdwdx.lua) *(cross platform)*<br>
Getting some information from PSD files.

- [quasiexpanderwdx.lua](plugins/wdx/scripts/quasiexpanderwdx.lua) *(cross platform)*<br>
Returns part of file name (by delimiter).

- [selinuxfilelabelswdx.lua](plugins/wdx/scripts/selinuxfilelabelswdx.lua)<br>
Getting SELinux file labels.

- [someaudioext4findfileswdx.lua](plugins/wdx/scripts/someaudioext4findfileswdx.lua) *(cross platform)*<br>
Checking if file(s) exists in folder by extension. List of file extensions (edit manually): table `extlist` in the beginning of script.<br>
For "Find files" dialog.

- [somefilesindirwdx.lua](plugins/wdx/scripts/somefilesindirwdx.lua) *(cross platform)*<br>
Like script above but for columns set or tooltips.

- [stringmatchwdx.lua](plugins/wdx/scripts/stringmatchwdx.lua)<br>
Plugin-example: search text in files, fields will create with patterns.

- [svginfowdx.lua](plugins/wdx/scripts/svginfowdx.lua) *(cross platform)*<br>
Getting some information from SVG files.

- [svninfowdx.lua](plugins/wdx/scripts/svninfowdx.lua)<br>
Getting some information from local copy of SVN repository. Requires `svn`.

- [symlinkwdx.lua](plugins/wdx/scripts/symlinkwdx.lua)<br>
Getting some details about symlinks.

- [tags](scripts/lua/tags) *(cross platform)*<br>
Assign any tag to any file or folder (local file systems), see readme.md.

- [tarfilescountwdx.lua](plugins/wdx/scripts/tarfilescountwdx.lua) + [tarfilescountwdx.lng](plugins/wdx/scripts/tarfilescountwdx.lng)<br>
Returns count of files, folders and symlinks it TAR files (list of file extension by default: tar, gz, bz2, lzma, xz, tgz, tbz, tbz2, tzma, tzx, tlz).

- [textsearchwdx.lua](plugins/wdx/scripts/textsearchwdx.lua) + [textsearchwdx.lng](plugins/wdx/scripts/textsearchwdx.lng)<br>
Search text in files. List of file extensions and used converters: table `commands` in the beginning of script.

- [tiffwdx.lua](plugins/wdx/scripts/tiffwdx.lua) *(cross platform)*<br>
Getting some information from TIFF files.

- [transmissionwdx.lua](plugins/wdx/scripts/transmissionwdx.lua)<br>
Returns information from TORRENT files. Requires `transmission-show`.

- [trashwdx.lua](plugins/wdx/scripts/trashwdx.lua) *(cross platform)*<br>
Getting info about deleted (moved to trash) files: original filename and deletion date. Use in a trash directory.

- [vcardinfowdx.lua](plugins/wdx/scripts/vcardinfowdx.lua) *(cross platform)*<br>
Getting some information from vCard files. See details in the beginning of script.

- [webpwdx.lua](plugins/wdx/scripts/webpwdx.lua) *(cross platform)*<br>
Getting some information from WebP files.

- [xcfinfowdx.lua](plugins/wdx/scripts/xcfinfowdx.lua) *(cross platform)*<br>
Getting some information from XCF files (GIMP native image format).

- [ziminfowdx.lua](plugins/wdx/scripts/ziminfowdx.lua) *(cross platform)*<br>
Getting some information from Zim files (https://zim-wiki.org/).

- [zsyncwdx.lua](plugins/wdx/scripts/zsyncwdx.lua) *(cross platform)*<br>
Getting some information from zsync-files.


---
<a name="wfx"><h3>WFX plugins</h3></a>

- [aur_crap](plugins/wfx/aur_crap)<br>
List of packages in [AUR](https://aur.archlinux.org/). (slow, pretty useless)

- [avfs_crap](plugins/wfx/avfs_crap)<br>
Simple wfx plugin for [AVFS](http://avf.sourceforge.net/).

- [clipboard](plugins/wfx/clipboard)<br>
View clipboard content. (GTK2)

- [clipboard_files](plugins/wfx/clipboard_files)<br>
View files in the clipboard. (GTK2)

- [cmdoutput](plugins/wfx/cmdoutput)<br>
View output of command line utilities (see `settings.ini`).

- [cmdoutput_panel](plugins/wfx/cmdoutput_panel)<br>
Like `cmdoutput` but it outputs data to a column in the file panel (DC 1.1+). Settings are stored in `$DC_CONFIG_PATH/j2969719.ini`.

- [contentfilter_crap](plugins/wfx/contentfilter_crap)<br>
Filter files by content (DC 1.1+).

- [desktopfiles](plugins/wfx/desktopfiles)<br>
Edit *.desktop files.

- [envlist](plugins/wfx/envlist)<br>
Just a list of the current environment variables (DC 1.1+).

- [fnmatch_crap](plugins/wfx/fnmatch_crap)<br>
Filter files by Unix pattern (DC 1.1+).

- [gtkrecent](plugins/wfx/gtkrecent)<br>
View list of recently used files (also support: open, view or delete any entry). (for GTK2-version of DC only)

- [gvfs_quickmount](plugins/wfx/gvfs_quickmount)<br>
Mounting gvfs resources with automatic password entry (stored in ini file). F7 - new connection, Alt+Enter - edit existing.

- [href_crap](plugins/wfx/href_crap)<br>
Shows href links from web page as files.

- [icasecopy](plugins/wfx/icasecopy)<br>
Case-insensitive Copy.

- [null_crap](plugins/wfx/null_crap)<br>
"Сopying" files to nowhere, to check if the file can still be read from some device. 

- [physfs_crap](plugins/wfx/physfs_crap)<br>
Virtual panel for [PhysicsFS](https://icculus.org/physfs/). To open the archive, copy the file to the panel.

- [proclst](plugins/wfx/proclst)<br>
View a list of running processes.

- [taglib_crap](plugins/wfx/taglib_crap)<br>
Virtual Panel for audio tags editing. (DC 1.1+).

- [tmppanel_crap](plugins/wfx/tmppanel_crap)<br>
Temporary panel, see FileList description.

- ~[trash_crap](plugins/wfx/trash_crap)<br>
View trash folder.~ (kinda useless and unfinished).

- [wfx_scripts](plugins/wfx/wfx_scripts)<br>
The WFX plugin allows to create some simple virtual file systems by scripting.


---
<a name="wlxgtk"><h3>WLX plugins (GTK2)</h3></a>

- [abiword-gtk2](plugins/wlx/abiword-gtk2)<br>
Displays DOC, RTF, DOT, ABW, AWT, ZABW. Requires AbiWord (GTK2 version).

- [atril-gtk2](plugins/wlx/atril-gtk2)<br>
Displays PDF, DjVu, TIFF, PostScipt, CBR, CBZ, XPS. Requires Atril 2 (GTK2 version).

- [csvview_gtk2](plugins/wlx/csvview_gtk2)<br>
Very primitive CSV viewer.

- ~[dirsize_crap](plugins/wlx/dirsize_crap)<br>
Display folder and file sizes in quick view.~ (kinda useless and unfinished).

- [evince2](plugins/wlx/evince2)<br>
Displays PDF, DjVu, TIFF, PostScipt, CBR. Requires Evince 2 (GTK2 version).

- [fileinfo](plugins/wlx/fileinfo)<br>
Displays various information about file using command line utilities. Requires `gtksourceview-2.0` ([original](https://github.com/doublecmd/doublecmd/wiki/Plugins#fileinfo)).

- [gstplayer](plugins/wlx/gstplayer)<br>
Simple media player plugin based on GStreamer framework. Few cosmetic changes, bit unstable ([original](https://github.com/doublecmd/doublecmd/wiki/Plugins#gstplayer)).

- [gtk_socket](plugins/wlx/gtk_socket)<br>
Plugin-wrapper for embed window of different scripts and utilities, used GtkSocket (container for widgets from other processes). Works with file extensions and MIME types, see `settings.ini`.

- [gtkimgview](plugins/wlx/gtkimgview)<br>
Image viewer plugin. Supported formats (all image formats supported by GdkPixbuf, the list can be differ on your system): xpm, xbm, tif, tiff, targa, tga, svg.gz, svgz, svg, qif, qtif, ppm, pgm, pbm, pnm, png, jpg, jpe, jpeg, jpf, j2k, jpx, jpc, jp2, cur, ico, icns, gif, bmp, ani.  Requires `gtkimageview` library.

- [gtkimgview_crap](plugins/wlx/gtkimgview_crap)<br>
Like `gtkimgview` but for any files if you can convert it to image (see `settings.ini`). Requires `gtkimageview` library.

- [gtksourceview](plugins/wlx/gtksourceview)<br>
Displays source code files with syntax highlighting. Requires `gtksourceview-2.0`.

- [jsonview_gtk2](plugins/wlx/jsonview_gtk2)<br>
Very primitive JSON Viewer.

- [hx_webkit_crap](plugins/wlx/hx_webkit_crap)<br>
Document viewer based on WebKitGTK2 and using HTML Export from Oracle Outside In Technology.

- [imagemagick](plugins/wlx/imagemagick)<br>
Image viewer plugin. Supported formats: dds, tga, pcx, bmp, webp.
If you have ImageMagick 6 (for example, Debian/Ubuntu-based distributions) then use `#include <wand/MagickWand.h>` instead `#include <MagickWand/MagickWand.h>`.

- [libarchive_crap](plugins/wlx/libarchive_crap)<br>
Displays file content with `libarchive` ([supported formats](https://github.com/libarchive/libarchive#supported-formats)).

- [libarchive_cat_crap](plugins/wlx/libarchive_cat_crap)<br>
Displays decompress data with `libarchive`.

- [md4c_webkit](plugins/wlx/md4c_webkit)<br>
This plugin allows you to view Markdown files.

- [mimescript](plugins/wlx/mimescript)<br>
Displays various information about file using command line utilities. Detection by MIME type.

- [mpv](plugins/wlx/mpv)<br>
Media player plugin. Requires `mpv`.

- [mpv_alt](plugins/wlx/mpv_alt)<br>
Yet another media player plugin. Requires `mpv`.

- [nfoview](plugins/wlx/nfoview)<br>
Displays NFO, DIZ.

- [sqlview_gtk2](plugins/wlx/sqlview_gtk2)<br>
Very primitive SQL Viewer.

- [symlinkerror](plugins/wlx/symlinkerror)<br>
Display symlink error.

- [wlxpview](plugins/wlx/wlxpview)<br>
PDF Viewer ([original](https://yassernour.wordpress.com/2010/04/04/how-hard-to-build-a-pdf-viewer/)).

- [wlxwebkit](plugins/wlx/wlxwebkit)<br>
This plugin allows you to view HTML/XHTML files. It is based on WebKitGTK2 engine.

- [wlxwebkit_crap](plugins/wlx/wlxwebkit_crap)<br>
Like `wlxwebkit` but for any files if you can convert it to HTML file (see `settings.ini`).

- [yet_another_vte_plugin](plugins/wlx/yet_another_vte_plugin)<br>
Embeds Virtual Terminal Emulator (VTE) widget (GTK2 version) and runs command line utilities (see `settings.ini`).<br>
More usable than `vte_in_quickview` or `vte_ncdu`.

- [zathura](plugins/wlx/zathura)<br>
Displays PDF, DjVu, PostScipt, CBR. Requires `zathura`.


---
<a name="wlxqt5"><h3>WLX plugins (Qt5)</h3></a>

- [bit7z_qt_crap](plugins/wlx/bit7z_qt_crap)<br>
Archive item props. (useless)

- [csvview_qt](plugins/wlx/csvview_qt)<br>
Very primitive CSV viewer.

- [dirchart_qml_qt_crap](plugins/wlx/dirchart_qml_qt_crap)<br>
Display file sizes in quick view.

- [dirextchart_crap_qt](plugins/wlx/dirextchart_crap_qt)<br>
Display file sizes by ext in `settings.ini` in quick view.

- [dirsize_crap_qt](plugins/wlx/dirsize_crap_qt)<br>
Display folder and file sizes in quick view.

- [fileinfo_qt](plugins/wlx/fileinfo_qt)<br>
Displays various information about file using command line utilities ([original](https://github.com/doublecmd/doublecmd/wiki/Plugins#fileinfo)).

- [fontview_qt](plugins/wlx/fontview_qt)<br>
Displays font from file.

- [htmlconv_qt_crap](plugins/wlx/htmlconv_qt_crap)<br>
Like [htmlview_qt_crap](plugins/wlx/htmlview_qt_crap) but for any files if you can convert it to HTML file (see `settings.ini`).

- [htmlview_qt_crap](plugins/wlx/htmlview_qt_crap)<br>
This plugin allows you to view HTML files.

- [hx_qt_crap](plugins/wlx/hx_qt_crap)<br>
Document viewer using HTML Export from Oracle Outside In Technology.

- [hx_webkit_qt_crap](plugins/wlx/hx_webkit_qt_crap)<br>
Document viewer based on Qt5WebKit and using HTML Export from Oracle Outside In Technology.

- [imgview_crap_qml_qt](plugins/wlx/imgview_crap_qml_qt)<br>
Very primitive image viewer.

- [imgview_crap_qml_qt_crap](plugins/wlx/imgview_crap_qml_qt_crap)<br>
Converts some file to image via some external tool assigned by mime or ext in `settings.ini` and displays the result.

- [jsonview_qt](plugins/wlx/jsonview_qt)<br>
Very primitive JSON Viewer.

- [libarchive_qt_crap](plugins/wlx/libarchive_qt_crap)<br>
Displays file content with `libarchive` ([supported formats](https://github.com/libarchive/libarchive#supported-formats)).

- [libarchive_cat_qt_crap](plugins/wlx/libarchive_cat_qt_crap)<br>
Displays decompress data with `libarchive`.

- [md4c_qt](plugins/wlx/md4c_qt)<br>
This plugin allows you to view Markdown files.

- [md4c_webkit_qt](plugins/wlx/md4c_webkit_qt)<br>
This plugin allows you to view Markdown files.

- [mpv_alt](plugins/wlx/mpv_alt)<br>
Yet another media player plugin. Requires `mpv`.

- [pdf_crap_qml_qt](plugins/wlx/pdf_crap_qml_qt)<br>
PDF Viewer.

- [qmediaplayer_qt](plugins/wlx/qmediaplayer_qt)<br>
Very primitive media player.

- [qtermwidget_qt_crap](plugins/wlx/qtermwidget_qt_crap)<br>
Embeds `qtermwidget` and runs command line utilities (see `settings.ini`).

- [qtpdfview_qt](plugins/wlx/qtpdfview_qt)<br>
PDF Viewer.

- [qtpdfview_qt_crap](plugins/wlx/qtpdfview_qt_crap)<br>
Converts some file to PDF via some external tool assigned by mime or ext in `settings.ini` and displays the result.

- [sqlview_qt](plugins/wlx/sqlview_qt)<br>
Very primitive SQL Viewer.

- [symlinkerror_qt](plugins/wlx/symlinkerror_qt)<br>
Display symlink error.

- [syntax-highlighting_qt](plugins/wlx/syntax-highlighting_qt)<br>
Displays source code files with syntax highlighting. Requires `syntax-highlighting` from KF5 framework.

- [webengine_qt](plugins/wlx/webengine_qt)<br>
This plugin allows you to view HTML/XHTML files. It is based on Blink engine.

- [wlxwebkit_qt](plugins/wlx/wlxwebkit_qt)<br>
This plugin allows you to view HTML/XHTML files. It is based on Qt5WebKit engine.

- [wlxwebkit_qt_crap](plugins/wlx/wlxwebkit_qt_crap)<br>
Like [wlxwebkit_qt](plugins/wlx/wlxwebkit_qt) but for any files if you can convert it to HTML file (see `settings.ini`).


---
<a name="dsx"><h3>DSX plugins</h3></a>

- [dsx_script](plugins/dsx/dsx_script)<br>
Search/feed to listbox via random command line utilities. Add some magic to `script.sh`.

- [git_ignored](plugins/dsx/git_ignored)<br>
Finds ignored files in git repository via `git ls-files -i --exclude-standard`.

- [git_modified](plugins/dsx/git_modified)<br>
Finds modified files in git repository via `git ls-files -m`.

- [git_untracked](plugins/dsx/git_untracked)<br>
Finds untracked files and other in git repository via `git ls-files -o`.

- [gtkrecent](plugins/dsx/gtkrecent)<br>
Feed to listbox recent files. (for GTK2-version of DC only)

- [locate_crap](plugins/dsx/locate_crap)<br>
Search via `locate` (w/o StartPath). 

- [lslocks](plugins/dsx/lslocks)<br>
List local system locks.

- [recollq_crap](plugins/dsx/recollq_crap)<br>
Full text search using [Recoll](https://www.lesbonscomptes.com/recoll/). 

- [tracker_textsearch](plugins/dsx/tracker_textsearch)<br>
Full text search using [Tracker](https://wiki.gnome.org/Projects/Tracker).
