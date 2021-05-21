Virtual file system scripts
=======
The plugin looks for executable files in this folder. The plugin compiled with the Temporary Panel API looks for executables in `temppanel` subfolder.

If a file like `SCRIPTFILENAME_readme.txt` exists in this folder, its contents will be displayed in the properties dialog instead of the first ten lines of the script.

# commands
- Each command is a separate script call with various arguments.
- Temporary data should not be stored in global variables.
- If called command is not implemented, script should return a nonzero exit status.
- The script must return exit status 0 **only** if called operation was successful, otherwise it must return any non-zero.
- Only **list** command is more or less mandatory.


## init
`SCRIPT init`

Initial initialization.

The script can output to stdout a line-by-line list of options. Custom options should not start with `Fs_`.

### List of reserved options

#### Fs_Request_Options

Line `Fs_Request_Options` signals that subsequent lines that do not start with `Fs_` are custom options and must be requested from the user.

#### Fs_StatusInfo_Needed

Outputting the reserved line `Fs_StatusInfo_Needed` activates the script calls with the **statusinfo** command.

#### Fs_GetSupportedField_Needed

Outputting the line `Fs_GetSupportedField_Needed` activates the script call with the getfields command the _next time the WFX plugin is initialized._ It is only needed if the script has fields to use in `Options -> Files views -> Columns -> Custom Columns`

#### Fs_Set_DC_WFX_SCRIPT_DATA/Fs_Set_DC_WFX_TP_SCRIPT_DATA

Outputting a line starting with `Fs_Set_DC_WFX_SCRIPT_DATA ` signals that the next part of the line should be set as a value for the `DC_WFX_SCRIPT_DATA` environment variable, which should be passed to the script between calls.
For plugin compiled with the Temporary Panel API, these are `Fs_Set_DC_WFX_TP_SCRIPT_DATA ` and `DC_WFX_TP_SCRIPT_DATA`, respectively.

#### Fs_MultiChoice

Outputting a line starting with `Fs_MultiChoice ` signals that the next part of the line contains custom option and multiple option values, separated by tabs to select one i.e. `Fs_MultiChoice OPTION\tVALUE1\tVALUE2\tVALUE3`.
After closing the dialog box, the script will be called with the `setopt OPTION SELECTED_VALUE`

#### Fs_YesNo_Message

Outputting a line starting with `Fs_YesNo_Message ` signals that the next part of the line should be shown in a dialog box with Yes/No buttons.
After closing the dialog box, the script will be called with the `setopt TEXT_DISPLAYED_IN_DIALOGBOX Yes` (ог `setopt TEXT_DISPLAYED_IN_DIALOGBOX No`) arguments.

#### Fs_Info_Message

Outputting a line starting with `Fs_Info_Message ` signals that the next part of the line should be shown in the info dialog box.

## deinit
`SCRIPT deinit`

Called when the script is changed or when Double Commaner is exited.

## setopt
`SCRIPT setopt OPTION VALUE`

Passes the user-supplied `VALUE` for one of the options requested for `Fs_Request_Options` to the script.

The output to stduot will be processed in the same way as for the **init** command.

## list
`SCRIPT list PATH`

Prints to stdout a list of files in the virtual folder specified in `PATH`.

One line should contain the attribute string, ISO 8601 UTC date (if Z is omitted, local time is expected), file size in bytes and file name, separated by whitespaces.

List example:
```
drwxr-xr-x 2021-04-11T08:59:34Z  - folder
-rw-r--r-- 2021-04-11T08:59:34Z   123456789  file.ext
```

## copyout
`SCRIPT copyout SOURCE DESTINATION`

Called to get a file from the virtual file system.

Copies a virtual file `SOURCE` to a local file `DESTINATION`

To update the progress bar, the script must print the integer percentage value line by line.

## copyin
`SCRIPT copyin SOURCE DESTINATION`

Called to put a file in the virtual file system.

Copies a local file `SOURCE` to a virtual file `DESTINATION`

To update the progress bar, the script must print the integer percentage value line by line.

## cp
`SCRIPT cp SOURCE DESTINATION`

Copies a virtual file `SOURCE` to `DESTINATION` inside the virtual file system.

To update the progress bar, the script must print the integer percentage value line by line.

## mv
`SCRIPT mv SOURCE DESTINATION`
Moves/Renames a virtual file `SOURCE` to `DESTINATION` inside the virtual file system.

To update the progress bar, the script must print the integer percentage value line by line.

## exists
`SCRIPT exists FILE`

Checks for the existence of a file on the virtual file system.

The script should return exit status 0 **only** if `FILE` exists.

## rm
`SCRIPT rm FILE`

Deletes the virtual file.

To update the progress bar, the script must print the integer percentage value line by line.

## mkdir
`SCRIPT mkdir PATH`

Creates a virtual directory.

The output to stduot will be processed in the same way as for the **init** command.

## rmdir
`SCRIPT rmdir PATH`

Removes the virtual directory.

## chmod
`SCRIPT chmod PATH MODE`

Called from Double Commander's dialog `Files -> Change Attributes...`

Changes the access rights of the virtual file system object at the specified `PATH`.

The argument `MODE` is sent in octal form.

## modtime
`SCRIPT modtime PATH DATETIME`

Called called when copying to virtual file system or from Double Commander's dialog `Files -> Change Attributes...`

Changes the date of the virtual file system object at the specified `PATH`.

The argument `DATETIME` is sent in ISO 8601 UTC form.

## run
`SCRIPT run FILE`

Called when the user double-clicks or presses Enter on a virtual file. Allows you to override default action.

In addition, if the script prints a string to standard output, it will be interpreted as the path to the directory to change to.

If the output to stduot starts with `Fs_`, it will be processed in the same way as for the **init** command.

## properties
`SCRIPT properties FILE`

Called when the user invokes file properties on a virtual file.

The script can print to stdout a line-by-line list of up to ten properties to display. Each line must contain the property name and value for the current `FILE`, separated by a tab.

Example:
```
Duration\t8:10
```

If the script did not output anything to stdout, but returned exit status 0, it is assumed that the properties dialog is in some form implemented by the script.

If the output to stduot starts with `Fs_`, it will be processed in the same way as for the **init** command.

## quote
`SCRIPT quote STRING PATH`

Called when the user enters some `STRING` at the command line below the panel.

## statusinfo
`SCRIPT statusinfo OPERATION PATH`

A WFX-specific call to inform the script about the start or end of an operation.
To reduce the number of script calls, it is suppressed by default. To activate script calls with this command, the script must print `Fs_StatusInfo_Needed` line at the **init** stage.

List of possible (not all of them are currently supported by Double Commander) `OPERATION` values
- `list start`
- `list end`
- `get_single start`
- `get_single end`
- `get_multi start`
- `get_multi end`
- `put_single start`
- `put_single end`
- `put_multi start`
- `put_multi end`
- `get_multi_thread start`
- `get_multi_thread end`
- `put_multi_thread start`
- `put_multi_thread end`
- `renmov_single start`
- `renmov_single end`
- `renmov_multi start`
- `renmov_multi end`
- `delete start`
- `delete end`
- `attrib start`
- `attrib end`
- `mkdir start`
- `mkdir end`
- `exec start`
- `exec end`
- `calcsize start`
- `calcsize end`
- `search start`
- `search end`
- `search_text start`
- `search_text end`
- `sync_search start`
- `sync_search end`
- `sync_get start`
- `sync_get end`
- `sync_put start`
- `sync_put end`
- `sync_delete start`
- `sync_delete end`

The output to stduot will be processed in the same way as for the **init** command.

## localname
`SCRIPT localname FILE`

Specific for Temporary Panel API.

Called to get the real location of the virtual file. The script should print the local path to stdout.

## getfields
`SCRIPT getfields`

Call to get a list of string fields to use in `Options -> Files views -> Columns -> Custom Columns`.
The script should print a line-by-line list of supported fields to stdout.

Attention, the plugin calls all available scripts that set `Fs_GetSupportedField_Needed` option previously, with this command _at the stage of its own initialization_, before calling the script with the **init** command.

## getvalue
`SCRIPT getvalue FIELD FILE`

Call to get the value of a `FIELD` for a virtual `FILE`. The script should print the value to stdout.