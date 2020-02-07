Marker (cross-platform)
-----------------------

2020.02.07

Marker for file highlighting like in *Colors* > *File types*, but "on the fly": add selected color, delete or change. So you can use this scripts for file tagging.
You can use any number of colors.

Written on Lua, see [DC Help > 2.10. Lua scripting > 2. DLL required](http://doublecmd.github.io/doc/en/lua.html#dllrequired).

This Marker is in two parts:

- `marker.lua` - for button(s): adding, deleting or changing color for selected files;
- `markerwdx.lua` - WDX-plugin part, for template(s)

List of marked files (will be used full paths!) will be save with name `marker.txt` near `marker.lua` and `markerwdx.lua`.

About color format: You can use any format, but format RGB is more easy for color settings editing and I use it for example.
Below I will use `Purple` as `128,0,128`.


## How to use

### Step 1: Add  buttons

How to use toolbar: [DC Help > 2.6. Toolbar](http://doublecmd.github.io/doc/en/toolbar.html)

(Menu is more useful, see note below.)

**_Adding or changing color for selected files_**

Add button with internal command `cm_ExecuteScript` and parameters
```
path/to/marker.lua
--add
%LU
128,0,128
```
where
- `%LU` - see [DC Help > 2.7. Variables in parameters > 9. List of files](http://doublecmd.github.io/doc/en/variables.html#listoffiles)
- `128,0,128` - color in selected format.

**_Deleting color for selected files_**

Add button with internal command `cm_ExecuteScript` and parameters
```
path/to/marker.lua
--del
%LU
```

### Step 2: Add WDX-plugin part

Go to `Configuration` > `Options...` > `Plugins` > `Plugins WDX` > `Add` > choose `markerwdx.lua`, go to `Colors` > `File types` and add new:

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

### Step 3: Now try

Select files and press your button :)

**NOTE:** Maybe you will need to update file list (press Ctrl+R or go to parent folder and return).
