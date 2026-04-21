gtk_socket_tst
========
Plugin-wrapper for embed window of different scripts and utilities, used GtkSocket (container for widgets from other processes).

![plug-preview](https://i.imgur.com/ZDu83VW.png)

## Compatibility
- `doublecmd-gtk2`
- `doublecmd-gtk3`

## Notes
Works with file extensions and MIME types, see `settings.ini`.
The settings file is read from `~/.local/share/doublecmd/plugins/wlx/gtk_socket_tst/settings.ini` or from `setting.ini` in the plugin directory.
If the path to the executable is not absolute, the search sequence during execution is: `~/.local/share/doublecmd/plugins/wlx/gtk_socket_tst/scripts/:`plugin directory`/scripts:PATH`

