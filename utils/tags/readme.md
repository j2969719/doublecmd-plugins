Tags (cross-platform)
-----------------------

2022.12.07

Assign any tag to any file or folder (local file systems). Tags can be shown in the column view or in the tooltip.
Also you can search by tag, select/unselect a group or use quick filter.

Written on Lua, see [Lua scripting > DLL required](http://doublecmd.github.io/doc/en/lua.html#dllrequired).

Two parts:

- `tags.lua` - for button(s): adding, deleting tags and so on;
- `tagswdx.lua` - WDX-plugin.

**NOTE:** The scripts use `os.setenv()`, so for DC < 1.1.0, use [previous versions](https://github.com/j2969719/doublecmd-plugins/tree/6fbeb3cdac25a5f556fd34c4fa9485914fca4e09/utils/tags).

The tags are stored in a file named `tags.txt` near `tags.lua` and `tagswdx.lua`. This is a plain text file (UTF-8 without BOM), so you can also edit its contents manually. Format:

```
full-(with-path)-name-of-file1|tag1|
full-(with-path)-name-of-file2|tag2|tag3|
```

## How to use

### Step 1: Add  buttons

How to use toolbar: [Toolbar](http://doublecmd.github.io/doc/en/toolbar.html). (Menu is more useful.)

Use internal command `cm_ExecuteScript` and the following parameters:

**_Add tag_**

```
path/to/tags.lua
--add
%LU
```

where `%LU` is the list of selected files, see [Variables in parameters](http://doublecmd.github.io/doc/en/variables.html).

**_Delete tag_**

```
path/to/tags.lua
--del
%LU
```

**_Delete all tags_**

```
path/to/tags.lua
--del-all
%LU
```

**_Change tag_**

```
path/to/tags.lua
--change
%"0%p0
```

where `%"0%p0` is the file under the cursor, see [Variables in parameters](http://doublecmd.github.io/doc/en/variables.html).

**_Quick filter_**

See [Quick search/filter](http://doublecmd.github.io/doc/en/help.html#cm_QuickSearch).

```
path/to/tags.lua
--filter
%"0%D
```

where `%"0%D` is the current directory in the active panel, see [Variables in parameters](http://doublecmd.github.io/doc/en/variables.html).

**_Auto reload file list_**

`--auto` is optional parameter, must be last: script will send DC the [cm_Refresh](https://doublecmd.github.io/doc/en/cmds.html#cm_Refresh) command.

### Step 2: Add WDX-plugin part

Go to `Configuration` > `Options...` > `Plugins` > `Plugins WDX` > `Add` > choose `tagswdx.lua`, now see:

[Custom columns](https://doublecmd.github.io/doc/en/configuration.html#ConfigColumns)

[Tooltips](https://doublecmd.github.io/doc/en/configuration.html#ConfigTooltips)

[Find files](https://doublecmd.github.io/doc/en/findfiles.html#plugins) & [Select/unselect a group](https://doublecmd.github.io/doc/en/help.html#mnu_select)

[Colors > File types](https://doublecmd.github.io/doc/en/configuration.html#ConfigColorFiles)

### Step 3: Try

**NOTE:** Maybe you will need to update file list: use `--auto` or press Ctrl+R (or, for example, go to the parent folder and return back).
