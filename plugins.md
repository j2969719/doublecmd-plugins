Plugins
-------

- [WCX plugins](#wcx)

- WDX plugins: [binary (compiled) plugins](#wdx-bin) and [Lua scripts](#wdx-lua)

- [WFX plugins](#wfx)

- WLX plugins: [GTK2](#wlxgtk) (for GTK2-version of DC only), [Qt5](#wlxqt5) (for Qt5-version of DC only)

- [DSX plugins](#dsx)

---
<a name="wcx"><h3>WCX plugins</h3></a>

- [cmdconv_crap](plugins/wcx/cmdconv_crap)<br>
Converts multiple files via commandline utilities (Files -> Pack Files...).

- [cmdoutput](plugins/wcx/cmdoutput)<br>
View output of command line utilities on Ctrl+Page Down (see `settings_default.ini`).

- [gcrypt_hash_crap](plugins/wcx/gcrypt_hash_crap)<br>
Calculates a hash (Files -> Pack Files...), test file integrity (Files -> Test Archive(s)).

- [libarchive_crap](plugins/wcx/libarchive_crap)<br>
A horrible attempt to make a simple WCX plugin based on `libarchive` ([supported formats](https://github.com/libarchive/libarchive#supported-formats) + [lizard](https://github.com/inikep/lizard)).

- [linkfiles_crap](plugins/wcx/linkfiles_crap)<br>
Creates hard or symbolic links for multiple files (Files -> Pack Files...).


---
<a name="wdx-bin"><h3>WDX: Binary (compiled) plugins</h3></a>

- [calcsize](plugins/wdx/calcsize)<br>
Folder search by size.

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

- ~[poppler_info](plugins/wdx/poppler_info)<br>
Basic information about PDF.~

- [simplechecksum](plugins/wdx/simplechecksum)<br>
Plugin is able to calculate and display hash values for any file. Can be used to search files by hash value.

- [simplefileinfo](plugins/wdx/simplefileinfo)<br>
A small wdx-plugin for Double Commander which provides additional information about the file via libmagic and stat.


---
<a name="wdx-lua"><h3>WDX: Lua scripts</h3></a>
Note: Some scripts uses command line utilities.

- [7zipwdx.lua](plugins/wdx/scripts/7zipwdx.lua)<br>
Getting some information (method, files and folder count, etc) from archives (using 7-Zip). You can see list of fields in `fields` (first column).

- [agewdx.lua](plugins/wdx/scripts/agewdx.lua) *(cross platform)*<br>
Returns age (the date of the last modification will be used): number of seconds, minutes, hours, days, weeks, months or years.

- [apkinfowdx.lua](plugins/wdx/scripts/apkinfowdx.lua)<br>
Getting some information from APK files. Fields: Label, Version, Install Location, Uses Permissions, Locales, Supports Screens, Native Code.

- [appcrashwdx.lua](plugins/wdx/scripts/appcrashwdx.lua) *(cross platform)*<br>
Getting info from CRASH-files on Debian/Ubuntu and Debian/Ubuntu-based distributions (/var/crash/*.crash). Only app crash files, not core dumps!

- [archivecommentwdx.lua](plugins/wdx/scripts/archivecommentwdx.lua)<br>
Reading comment from ZIP and RAR files.

- [bintstwdx.lua](plugins/wdx/scripts/bintstwdx.lua)<br>
Checking: binary or Unicode text file (encoding UTF-8, UTF-16 or UTF-32 and byte order, detection by BOM).

- [caseduplwdx.lua](plugins/wdx/scripts/caseduplwdx.lua) *(cross platform)*<br>
Search for duplicates with the same name but with a different case.

- [checkfileextwdx.lua](plugins/wdx/scripts/checkfileextwdx.lua) *(cross platform)*<br>
Checking that the file extension matches the file type (by the file signatures) and returns some additional info. See details in the beginning of script.

- [checkfilenamewdx.lua](plugins/wdx/scripts/checkfilenamewdx.lua) *(cross platform)*<br>
Check filename limitations and recommendations, path lenght and other, see description in the beginning of script.

- [crapxmlwdx.lua](plugins/wdx/scripts/crapxmlwdx.lua)<br>
Getting some information from FB2 files ([more complete alternative](https://github.com/doublecmd/plugins/tree/master/wdx/fb2wdx)).

- [descriptionwdx.lua](plugins/wdx/scripts/descriptionwdx.lua)<br>
Script reads file descriptions from `descript.ion`.

- [dfwdx.lua](plugins/wdx/scripts/dfwdx.lua)<br>
Mount points: name, FS type, all/used/available size and other.

- [diffwdx.lua](plugins/wdx/scripts/diffwdx.lua)<br>
Search in diff file with reference file or diff from Git or SVN repository.

- [djvuwdx.lua](plugins/wdx/scripts/djvuwdx.lua)<br>
Getting some information from DjVu files and searching text. Requires `djvused`.

- [duwdx.lua](plugins/wdx/scripts/duwdx.lua)<br>
Returns directory size or files on a file system.

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
Returns file or directory count, without scanning symbolic links to folders.<br>
For columns set only!

- [filenamechrstatwdx.lua](plugins/wdx/scripts/filenamechrstatwdx.lua) *(cross platform)*<br>
Returns count of characters in file name: total, alphanumeric, spaces, letters (all, lower case or upper case letters), digits and punctuation characters. You can choose the full path, path, file name or file extension.

- [filenamematchwdx.lua](plugins/wdx/scripts/filenamematchwdx.lua) *(cross platform)*<br>
Returns part of file name for sorting (getting title, volume, chapter, etc).

- [filenamereplacewdx.lua](plugins/wdx/scripts/filenamereplacewdx.lua) *(cross platform)*<br>
Returns new file name after replacing total or part of file name by the list (see and edit `RplFiles` in the beginning of script).

- [filetimeindirwdx.lua](plugins/wdx/scripts/filetimeindirwdx.lua) *(cross platform)*<br>
Returns date and time of oldest and newest files in directory. See details in the beginning of script.<br>
For columns set only!

- [fixfilenamewdx.lua](plugins/wdx/scripts/fixfilenamewdx.lua)<br>
Fixing file name encoding with `iconv`.

- [getfaclwdx.lua](plugins/wdx/scripts/getfaclwdx.lua)<br>
Returns file permissions (ACL). Requires `getfacl`.

- [getfattrwdx.lua](plugins/wdx/scripts/getfattrwdx.lua)<br>
Getting MS-DOS attributes in a FAT file system. Requires `getfattr`.

- [getfattrcustomwdx.lua](plugins/wdx/scripts/getfattrcustomwdx.lua)<br>
Getting custom extended attributes. Requires `getfattr`.

- [gitinfowdx.lua](plugins/wdx/scripts/gitinfowdx.lua)<br>
Getting some information from local copy of Git repository. Requires `git`.

- [hexdecheaderwdx.lua](plugins/wdx/scripts/hexdecheaderwdx.lua) and [hexheaderwdx.lua](plugins/wdx/scripts/hexheaderwdx.lua) *(cross platform)*<br>
Examples: search by hex value. Now Double Commander can do it itself.

- [icalendarwdx.lua](plugins/wdx/scripts/icalendarwdx.lua) *(cross platform)*<br>
Getting some information from iCalendar files.

- [infilelistwdx.lua](plugins/wdx/scripts/infilelistwdx.lua)<br>
Search file name in custom list of file names.

- [jpeginfowdx.lua](plugins/wdx/scripts/jpeginfowdx.lua) *(cross platform)*<br>
Getting some information from JPEG files. See details in the beginning of script.

- [libextractorwdx.lua](plugins/wdx/scripts/libextractorwdx.lua)<br>
Returns metadata from files. You can see list of fields in `fields` (first column). Requires `extract`.

- [lnkinfowdx.lua](plugins/wdx/scripts/lnkinfowdx.lua)<br>
Getting some information from Windows shell link (shortcut) file. Requires `lnkinfo`.

- [lslockswdx.lua](plugins/wdx/scripts/lslockswdx.lua)<br>
List local system locks.

- [modiftimeistodaywdx.lua](plugins/wdx/scripts/modiftimeistodaywdx.lua) *(cross platform)*<br>
The last modified time is today (not last twenty-four hours), boolean.

- [msgfulltextwdx.lua](plugins/wdx/scripts/msgfulltextwdx.lua) *(cross platform)*<br>
Returns full text of saved email messages. For Find files with plugins only! See details in the beginning of script.

- [msginfowdx.lua](plugins/wdx/scripts/msginfowdx.lua) *(cross platform)*<br>
Getting info from headers of saved email messages. Supported formats: *.eml, *.msg and MH mailfile format (Sylpheed and other).

- [pkginfowdx.lua](plugins/wdx/scripts/pkginfowdx.lua)<br>
Returns some information from Arch Linux packages.

- [poheaderwdx.lua](plugins/wdx/scripts/poheaderwdx.lua) *(cross platform)*<br>
Getting some information from PO-files (gettext).

- [quasiexpanderwdx.lua](plugins/wdx/scripts/quasiexpanderwdx.lua) *(cross platform)*<br>
Returns part of file name (by delimiter).

- [randomcharswdx.lua](plugins/wdx/scripts/randomcharswdx.lua) *(cross platform)*<br>
Returns random alphanumeric character(s) (case-sensitive), by default up to 10.

- [ru2enutf8wdx.lua](plugins/wdx/scripts/ru2enutf8wdx.lua) *(cross platform)*<br>
Converts keyboard layout Ru <> En. Requires `luautf8` module.

- [ru2enwdx.lua](plugins/wdx/scripts/ru2enwdx.lua) *(cross platform)*<br>
Converts keyboard layout Ru <> En.

- [scrtiptfileinfowdx.lua](plugins/wdx/scripts/scrtiptfileinfowdx.lua)<br>
Returns various information about file using command line utilities. Uses [fileinfo.sh](scripts/fileinfo.sh).

- [selinuxfilelabelswdx.lua](plugins/wdx/scripts/selinuxfilelabelswdx.lua)<br>
Getting SELinux file labels.

- [someaudioext4findfileswdx.lua](plugins/wdx/scripts/someaudioext4findfileswdx.lua) *(cross platform)*<br>
Checking if file(s) exists in folder by extension. List of file extensions (edit manually): table `extlist` in the beginning of script.<br>
For "Find files" dialog.

- [somefilesindirwdx.lua](plugins/wdx/scripts/somefilesindirwdx.lua) *(cross platform)*<br>
Like script above but for columns set.

- [sqlitetxtsearchwdx.lua](plugins/wdx/scripts/sqlitetxtsearchwdx.lua) *(cross platform)*<br>
Plugin-example: getting text from SQLite3 base. Requires `lsqlite3` module.

- [stringmatchwdx.lua](plugins/wdx/scripts/stringmatchwdx.lua)<br>
Plugin-example: search text in files, fields will create with patterns.

- [svginfowdx.lua](plugins/wdx/scripts/svginfowdx.lua) *(cross platform)*<br>
Getting some information from SVG files.

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

- [vcardinfowdx.lua](plugins/wdx/scripts/vcardinfowdx.lua) *(cross platform)*<br>
Getting some information from vCard files. See details in the beginning of script.

- [xcfinfowdx.lua](plugins/wdx/scripts/xcfinfowdx.lua) *(cross platform)*<br>
Getting some information from XCF files (GIMP native image format).

- [ziminfowdx.lua](plugins/wdx/scripts/ziminfowdx.lua) *(cross platform)*<br>
Getting some information from Zim files (https://zim-wiki.org/).


---
<a name="wfx"><h3>WFX plugins</h3></a>

- [adbplugin-x64](plugins/wfx/adbplugin-x64)<br>
PKGBUILD file for [Android ADB](http://www.uniqtec.eu/applications/android-adb.html).

- [avfs_crap](plugins/wfx/avfs_crap)<br>
Simple wfx plugin for [AVFS](http://avf.sourceforge.net/).

- [clipboard](plugins/wfx/clipboard)<br>
View clipboard content. (for GTK2-version of DC only)

- [cmdoutput](plugins/wfx/cmdoutput)<br>
View output of command line utilities (see `settings.ini`).

- [filelist](plugins/wfx/filelist)<br>
Temporary panel, virtual folder that allows keeping links to frequently used files.

- [gtkrecent](plugins/wfx/gtkrecent)<br>
View list of recently used files (also support: open, view or delete any entry). (for GTK2-version of DC only)

- [gvfs_quickmount](plugins/wfx/gvfs_quickmount)<br>
Mounting gvfs resources with automatic password entry (stored in ini file). F7 - new connection, Alt+Enter - edit existing.

- [physfs_crap](plugins/wfx/physfs_crap)<br>
Virtual panel for [PhysicsFS](https://icculus.org/physfs/). To open the archive, copy the file to the panel.

- [proclst](plugins/wfx/proclst)<br>
View a list of running processes.

- [tmppanel_crap](plugins/wfx/tmppanel_crap)<br>
Temporary panel, see FileList description.


---
<a name="wlxgtk"><h3>WLX plugins (GTK2)</h3></a>

- [abiword-gtk2](plugins/wlx/abiword-gtk2)<br>
Displays DOC, RTF, DOT, ABW, AWT, ZABW. Requires AbiWord (GTK2 version).

- [atril-gtk2](plugins/wlx/atril-gtk2)<br>
Displays PDF, DjVu, TIFF, PostScipt, CBR, CBZ, XPS. Requires Atril 2 (GTK2 version).

- [dcpoppler](plugins/wlx/dcpoppler)<br>
PDF Viewer ([original](https://yassernour.wordpress.com/2010/04/04/how-hard-to-build-a-pdf-viewer/)).

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

- [fileinfo_qt](plugins/wlx/fileinfo_qt)<br>
Displays various information about file using command line utilities ([original](https://github.com/doublecmd/doublecmd/wiki/Plugins#fileinfo)).

- [hx_webkit_qt_crap](plugins/wlx/hx_webkit_qt_crap)<br>
Document viewer based on Qt5WebKit and using HTML Export from Oracle Outside In Technology.

- [syntax-highlighting_qt](plugins/wlx/syntax-highlighting_qt)<br>
Displays source code files with syntax highlighting. Requires `syntax-highlighting` from KF5 framework.

- [wlxwebkit_qt5](plugins/wlx/wlxwebkit_qt5)<br>
This plugin allows you to view HTML/XHTML files. It is based on Qt5WebKit engine.


---
<a name="dsx"><h3>DSX plugins</h3></a>

- [dsx_script](plugins/dsx/dsx_script)<br>
Search/feed to listbox via random command line utilities. Add some magic to `script.sh`.

- [duplicates_crap](plugins/dsx/duplicates_crap)<br>
Find duplicates using `find` and `b2sum`. Slow, doublecmd's window may hang during search.

- [git_ignored](plugins/dsx/git_ignored)<br>
Finds ignored files in git repository via `git ls-files -i --exclude-standard`.

- [git_modified](plugins/dsx/git_modified)<br>
Finds modified files in git repository via `git ls-files -m`.

- [git_untracked](plugins/dsx/git_untracked)<br>
Finds untracked files and other in git repository via `git ls-files -o`.

- [gtkrecent](plugins/dsx/gtkrecent)<br>
Feed to listbox recent files. (for GTK2-version of DC only)

- [hardlinks_crap](plugins/dsx/hardlinks_crap)<br>
Find hardlinks using `find`. Slow, doublecmd's window may hang during search.

- [in_filelist](plugins/dsx/in_filelist)<br>
Feed to listbox files from filelist.txt.

- [locate_crap](plugins/dsx/locate_crap)<br>
Search via `locate` (w/o StartPath). 

- [lslocks](plugins/dsx/lslocks)<br>
List local system locks.

- [pacman_pkg_list](plugins/dsx/pacman_pkg_list)<br>
Feed to listbox local files of some installed archlinux package via `pacman`. Accepts only full pkgname in FileMask.

- [the_silver_searcher_crap](plugins/dsx/the_silver_searcher_crap)<br>
Pattern matching using `the_silver_searcher`. FileMask also accepts only regexp(if filename pattern is specified, search for part of filename option must be disabled).

- [tracker_textsearch](plugins/dsx/tracker_textsearch)<br>
Full text search using [Tracker](https://wiki.gnome.org/Projects/Tracker). 
