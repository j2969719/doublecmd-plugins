Virtual file system scripts
===========================
The plugin looks for executable files in this folder. The plugin compiled with the Temporary Panel API looks for executables in `temppanel` subfolder.

If the `no_dialog` value is `true` in the plugin configuration file or the `dialog.lfm` file is missing, the last used script will always be executed (`last_script` value).

# commands
- Each command is a separate script call with various arguments.
- Temporary data should not be stored in global variables.
- If called command is not implemented, script should return a nonzero exit status.
- The script must return exit status 0 **only** if called operation was successful, otherwise it must return any non-zero.
- Only **list** command is more or less mandatory.


## init
`SCRIPT init`

Initial initialization.

The script can output to stdout a line-by-line list of options to be requested. Outputting the `Disable_FsStatusInfo` line to stdout suppresses calls to the script with the **statusinfo** command.

## deinit
`SCRIPT deinit`

Called when the script is changed or when Double Commaner is exited.

## setopt
`SCRIPT setopt OPTION VALUE`

Passes the user-supplied `VALUE` for one of the options requested by **init** or **properties** to the script.

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

## copyin
`SCRIPT copyin SOURCE DESTINATION`

Called to put a file in the virtual file system.

Copies a local file `SOURCE` to a virtual file `DESTINATION`

## cp
`SCRIPT cp SOURCE DESTINATION`

Copies a virtual file `SOURCE` to `DESTINATION` inside the virtual file system.

## mv
`SCRIPT mv SOURCE DESTINATION`
Moves/Renames a virtual file `SOURCE` to `DESTINATION` inside the virtual file system.

## exists
`SCRIPT exists FILE`

Checks for the existence of a file on the virtual file system.

The script should return exit status 0 **only** if `FILE` exists.

## rm
`SCRIPT rm FILE`

Deletes the virtual file.

## mkdir
`SCRIPT mkdir PATH`

Creates a virtual directory.

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

## properties
`SCRIPT properties FILE`

Called when the user invokes file properties on a virtual file.

The script can output to stdout a line-by-line list of options to be requested. Outputting the `Disable_FsStatusInfo` line to stdout suppresses calls to the script with the **statusinfo** command.

## quote
`SCRIPT quote STRING`

Called when the user enters some `STRING` at the command line below the panel.

## statusinfo
`SCRIPT statusinfo OPERATION PATH`

A WFX-specific call to inform the script about the start or end of an operation.
To reduce the number of script calls, it can be suppressed by printing `Disable_FsStatusInfo` to stdout when invoking the **init** or **properties** commands.


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

## localname
`SCRIPT localname FILE`

Specific for Temporary Panel API.

Called to get the real location of the virtual file. The script should print the local path to stdout.

## getfields
`SCRIPT getfields`

Call to get a list of string fields to use in `Options -> Files views -> Columns -> Custom Columns`.
The script should print a line-by-line list of supported fields to stdout.

## getvalue
`SCRIPT getvalue FIELD FILE`

Call to get the value of a `FIELD` for a virtual `FILE`. The script should print the value to stdout.