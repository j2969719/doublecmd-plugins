mpv_alt
========
Media player plugin.

## Compatibility
- `doublecmd-gtk2`
- `doublecmd-qt5`
- `doublecmd-qt6`

## Notes
Requires `libmpv`.
The plugin uses the `dlopen` function instead of dynamic linking. By default, the plugin will try to use the file `libmpv.so` You can specify the library name in `j2969719.ini` in the folder with the Double Commander configuration files (see [Location of configuration files](https://doublecmd.github.io/doc/en/configuration.html#ConfigDC)):
```ini
[PluginName]
libpath=libmpv.so
```
Usually the library name is `libmpv.so.2` (mpv 0.35+) or `libmpv.so.1` (mpv <= 0.34).

## Previous version
[Previous version](https://github.com/j2969719/doublecmd-plugins/tree/8b89e4f9ae886bb3029fd4103a19c6bb1d343dc7/plugins/wlx/mpv_alt) is dynamically linked to the `libmpv` library and it has a problem with the Lua library, see [WLX Plugin mpv_alt liblua conflict #43](https://github.com/j2969719/doublecmd-plugins/issues/43).
In the [Double Commander settings](https://doublecmd.github.io/doc/en/configuration.html#ConfigPlugins), you must specify the Lua library of the same version that mpv uses (see the package dependencies in your Linux distribution): Lua 5.1/LuaJIT (the LuaJIT library fully supports Lua 5.1 API) or Lua 5.2.
