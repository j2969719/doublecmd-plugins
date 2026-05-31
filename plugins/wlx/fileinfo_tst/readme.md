fileinfo
========
Displays various information about file using command line utilities.

![plug-preview](https://i.imgur.com/Fzorsfi.png)

## Compatibility
- `doublecmd-gtk2`
- `doublecmd-gtk3`

## Notes
([original](https://github.com/doublecmd/doublecmd/wiki/Plugins#fileinfo))

Works with file extensions and MIME types, see `settings.ini`.

`fileinfo_srcvw.wlx` requires `gtksourceview-2.0`.

The settings file is read from `~/.local/share/doublecmd/plugins/wlx/fileinfo_tst/settings.ini` or from `setting.ini` in the plugin directory.
If the path to the executable is not absolute, the search sequence during execution is: `~/.local/share/doublecmd/plugins/wlx/fileinfo_tst/scripts/:`plugin directory`/scripts:PATH`

`mc_script` in `setting.ini` to use those midnight commander scripts from `ext.d` directories


## Dependencies
![arch](https://wiki.archlinux.org/favicon.ico) [gtksourceview2](https://aur.archlinux.org/packages/gtksourceview2)
