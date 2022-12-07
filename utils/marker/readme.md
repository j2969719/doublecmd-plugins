Marker (cross-platform)
-----------------------

2022.12.07

Marker for file highlighting like in *Colors* > *File types*, but "on the fly": add selected color, delete or change. So you can use this scripts for file tagging.
You can use any number of colors.

Written on Lua, see [Lua scripting > DLL required](http://doublecmd.github.io/doc/en/lua.html#dllrequired).

Two parts:

- `marker.lua` - for button(s): adding, deleting or changing color for selected files;
- `markerwdx.lua` - WDX-plugin.

**NOTE:** The scripts use `os.setenv()`, so for DC < 1.1.0, use [previous versions](https://github.com/j2969719/doublecmd-plugins/tree/6fbeb3cdac25a5f556fd34c4fa9485914fca4e09/utils/marker).

List of marked files is stored in a file named `marker.txt` near `marker.lua` and `markerwdx.lua`. This is a plain text file (UTF-8 without BOM), so you can also edit its contents manually.

About color format: You can use any format, but format RGB is more easy for color settings editing and I use it for example.
Below I will use `Purple` as `128,0,128`.

## How to use

### Step 1: Add  buttons

How to use toolbar: [Toolbar](http://doublecmd.github.io/doc/en/toolbar.html). (Menu is more useful, see note below.)

Use internal command `cm_ExecuteScript` and the following parameters:

**_Adding or changing color for selected files_**

```
path/to/marker.lua
--add
%LU
128,0,128
```

where

- `%LU` is the list of selected files, see [Variables in parameters](http://doublecmd.github.io/doc/en/variables.html);

- `128,0,128` - color in selected format.

**_Deleting color for selected files_**

```
path/to/marker.lua
--del
%LU
```

### Step 2: Add WDX-plugin part

Go to `Configuration` > `Options...` > `Plugins` > `Plugins WDX` > `Add` > choose `markerwdx.lua`, go to `Colors` > [File types](http://doublecmd.github.io/doc/en/configuration.html#ConfigColorFiles) and add new:

- press `Add` and write `Category name`;

- `Category mask`: press buttom `Templates...` > `Define...` > go to tab `Plugins`;

- choose `Use content plugins, combine with:` and `AND (all matches)`;

- add rule:<br>
plugin: `markerwdx`<br>
field: `Color`<br>
operator: `=(case)`<br>
value: `128,0,128`

- press `Save`;

- choose template name, now `Ok`;

- now choose color in `Category color`: press button `>>` and write<br>
    red: `128`<br>
    green: `0`<br>
    blue: `128`

- press `Apply` (both).

**NOTE:** You must add buttons and file types (step 1 and step 3) for ALL desired colors!

### Step 3: Try

**NOTE:** Maybe you will need to update file list (press Ctrl+R or go to the parent folder and return back).
