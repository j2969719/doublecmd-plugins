Virtual file system scripts
=======
The plugin looks for executable files in this folder. The plugin compiled with the Temporary Panel API looks for executables in `temppanel` subfolder.

For any operation, a executable script is launched with certain parameters and only if successful, it should return an exit status of 0. If the operation is not supported or an error occurs, the script must return a non-zero exit status. All other communication is done by printing text to stdout. A virtual file system is considered valid if, when running the script with the list / arguments, the script returns an exit status of 0.

# Supported operations

|Operation|arg1|arg2|arg3|Notes|
|---|---|---|---|---|
|Initialization|`init`|-|-|
|Finishing work with VFS|`deinit`|-|-||
|Retrieved value for option|`setopt`|text of requested option|received value||
|Requesting a filelist from a certain VFS directory|`list`|path of the virtual directory|-||
|Copying a virtual file from the VFS|`copyout`|source path of the virtual file|destination path of the local file||
|Copying a file into the VFS|`copyin`|source path of the local file|destination path of the virtual file||
|Copying a virtual file within the VFS|`cp`|source path of the virtual file|destination path of the virtual file||
|Moving a virtual file within the VFS|`mv`|source path of the virtual file|destination path of the virtual file||
|Checking whether a file exists within the VFS|`exists`|path of the virtual file|-|used to display the overwrite dialog.  The script must return exit status 0 only if the virtual file exists.
|Deleting a virtual file from the VFS|`rm`|path of the virtual file|-||
|Creating a virtual directory in a VFS|`mkdir`|path of the virtual directory|-||
|Removing a virtual directory from the VFS|`rmdir`|path of the virtual directory|-||
|Changing virtual file mode bits|`chmod`|path of the virtual file|mode in octal form|Files -> Change Attributes...|
|Changing virtual file modification time|`modtime`|path of the virtual file|datetime in ISO 8601 UTC form|Files -> Change Attributes...|
|Executing of a virtual file|`run`|path of the virtual file|-||
|Showing the properties dialog|`properties`|path of the virtual file|-||
|Processing user input from the command line under the file panel|`quote`|user input|path of the virtual directory|user input from commandline below the filepanel|
|Retrieving the value for an additional info column in the file panel|`getvalue`|path of the virtual file|-|not used by default, can be activated with `Fs_GetValue_Needed`|
|Retrieving all values for an additional info column in the file panel|`getvalues`|path of the virtual directory|-|not used by default, can be activated with `Fs_GetValues_Needed`|
|Getting the real filename if virtual files are links to local files|`localname`|path of the virtual file|-|only for plugin compiled with the Temporary Panel API|
|Reset saved settings|`reset`|-|-||
|Sending information to the script about the start/end of operations|`statusinfo`|operation status (list of possible values below)|path of the virtual directory|not used by default, can be activated with `Fs_StatusInfo_Needed`|

### List of possible statusinfo arg2 values
|Start|End|Supported|
|---|---|---|
|`list start`|`list end`|[x]|
|`get_single start`|`get_single end`|[ ]|
|`get_multi start`|`get_multi end`|[ ]|
|`put_single start`|`put_single end`|[ ]|
|`put_multi start`|`put_multi end`|[ ]|
|`get_multi_thread start`|`get_multi_thread end`|[x]|
|`put_multi_thread start`|`put_multi_thread end`|[x]|
|`renmov_single start`|`renmov_single end`|[x]|
|`renmov_multi start`|`renmov_multi end`|[x]|
|`delete start`|`delete end`|[x]|
|`attrib start`|`attrib end`|[x]|
|`mkdir start`|`mkdir end`|[x]|
|`exec start`|`exec end`|[x]|
|`calcsize start`|`calcsize end`|[x]|
|`search start`|`search end`|[ ]|
|`search_text start`|`search_text end`|[ ]|
|`sync_search start`|`sync_search end`|[ ]|
|`sync_get start`|`sync_get end`|[ ]|
|`sync_put start`|`sync_put end`|[ ]|
|`sync_delete start`|`sync_delete end`|[ ]|

## Expected script output to stdout

Operations **reset**, **exists**, **chmod** and **modtime** ignore output to stdout.


Operations **copyout**, **copyin**, **cp**, **mv**, **rm** and **rmdir** waits for a line-by-line digital value from 0 to 100 of the progress of the current operation to update the progressbar in the displayed window.


**list** operation expects a line-by-line list of files for the specified virtual folder.

Example:
```
drwxr-xr-x 2021-04-11T08:59:34Z  - folder
-rw-r--r-- 2021-04-11T08:59:34Z   123456789  file.ext
100644 0000-00-00 00:00:00 - another file.ext
```


**properties** operation expect a line-by-line list of property-value tab-separated pairs. A property starting with `png:` and containing the full path to a PNG image in its value will display this image in the properties window. The reserved properties `url`, `filetype` or `content_type` and `path` and abb allow you to assign the url, type and path values ​​of predefined fields in the properties window accordingly.
In addition, if one of these lines (but not the first one) starting with `Fs_PropsActs ` it signals that the next part of the line will contain a list of custom actions, separated by tabs (`Fs_PropsActs ACTION1\tACTION2\tACTION3`), that can be performed on the selected file.
When you select a custom action in the properties dialog, the script will be launched with these arguments: `setopt ACTION FILE`

Example:
```
pkgname	abiword
pkgdesc	Fully-featured word processor
png:icon	/usr/share/icons/hicolor/48x48/apps/abiword.png
pkgver	3.0.5-2
installed	3.0.5-2
size	24M (24 339 655)
builddate	2021-12-03 02:43:45
url	https://www.abisource.com
Fs_Set_DC_WFX_TP_SCRIPT_DATA abiword
Fs_PropsActs Install package	Show installed package info
```
If the script did not output anything to stdout, but returned exit status 0, it is assumed that the properties dialog is in some form implemented by the script. If the output to stduot starts with Fs_, it will be processed in the same way as for operations below.


Operations **init**, **deinit**, **setopt**, **mkdir**, **quote** and **statusinfo** support additional actions or user interaction through the use of reserved strings starting with fs_.  Below is a list of such strings.

### Reserved strings

|Reserved|Description|Possible arg3 for setopt|Example|Script call example|
|---|---|---|---|---|
|`Fs_Request_Options`|informs that the following lines are the values ​​to be prompted from the user|-|`Fs_Request_Options`|-|
|`Fs_StatusInfo_Needed`|will activate additional `statusinfo` calls for the script|-|`Fs_StatusInfo_Needed`|-|
|`Fs_GetValue_Needed`|will activate additional `getvalue` calls for the script|-|`Fs_GetValue_Needed`|-|
|`Fs_GetValues_Needed`|will activate additional `getvalues` calls for the script|-|`Fs_GetValues_Needed`|-|
|`Fs_ExecFeedback_Needed`|will enable additional `setopt` calls for the script after running commands using|-|`Fs_ExecFeedback_Needed`|-|
|`Fs_CONNECT_Needed`|informs that the plugin must send CONNECT to display the drive button and the log window|-|`Fs_CONNECT_Needed`|-|
|`Fs_DisableFakeDates`|disable substitution of an invalid date in the list of files for the current one|-|`Fs_DisableFakeDates`|-|
|`Fs_RequestOnce`|informs that the following lines are values ​​that are requested from the user only once|-|`Fs_RequestOnce`|-|
|`Fs_MultiChoice`|shows a dialog with several options to choose from|user input|`Fs_MultiChoice some option\tvalue1\tvalue2`|setopt some option value2|
|`Fs_YesNo_Message`|shows a dialog with yes/no buttons|`Yes`, `No`|`Fs_YesNo_Message some message`|setopt some message No|
|`Fs_Info_Message`|shows a dialog with an ok button|-|`Fs_Info_Message some message`|-|
|`Fs_PushValue`|sets the initial value before prompting the user|-|`Fs_PushValue startpath\t/home/user`|-|
|`Fs_SelectFile`|shows file selection dialog|user input|`Fs_SelectFile export file\tlog files|*.log|all files|*.*\tsave\tlog`|setopt export file /tmp/myfile.log|
|`Fs_SelectDir`|shows directory selection dialog|user input|`Fs_SelectDir workdir`|setopt workdir /tmp|
|`Fs_LogInfo`|informs that the following lines must be shown in the log window|-|`Fs_LogInfo`|-|
|`Fs_RunTerm`|executes command in terminal|`OK`, `Error`|`Fs_RunTerm ping 8.8.8.8`|setopt Fs_RunTerm ping 8.8.8.8 OK|
|`Fs_RunTermKeep`|executes a command in the terminal and keeps the terminal window open|`OK`, `Error`|`Fs_RunTermKeep ping 8.8.8.8`|setopt Fs_RunTermKeep ping 8.8.8.8 OK|
|`Fs_Open`|opens the file in the associated program using `xdg-open` or runs the executable file|`OK`, `Error`|`Fs_Open http://google.com`|setopt Fs_Open http://google.com OK|
|`Fs_OpenTerm`|opens the file in the associated program using `xdg-open` or runs the executable file in the terminal|`OK`, `Error`|`Fs_OpenTerm /bin/7z`|setopt Fs_OpenTerm /bin/7z Error|
|`Fs_RunAsync`|executes the command asynchronously|`OK`, `Error`|`Fs_RunAsync mousepad`|setopt Fs_RunAsync mousepad Error|
|`Fs_ShowOutput`|executes the command and shows a dialog with the received stdout|`Error`|`Fs_ShowOutput man fish`|setopt Fs_ShowOutput man fish Error|
|`Fs_Set_`ENVVAR|informs that in subsequent calls to the script the environment variable ENVVAR must be set to the specified value|-|`Fs_Set_DC_WFX_SCRIPTDATA myvalue`|-|


**run** operation can expect a line that contains path to the directory to change to. If the output to stduot starts with Fs_, it will be processed in the same way as for operations abowe.


**localname** operation expect a line that contains path to the local file.


**getvalue** operation expect a line that contains value for additonal column in filepanel.


**getvalue** operation expect a line-by-line list of file-value tab-separated pairs.

Example:
```
filename1.ext	link
filename	group
filename2.ext	-
```

## Reserved environment variables

|ENVVAR|Description|
|---|---|
|`DC_WFX_SCRIPT_REMOTENAME`|stores the vfs path for the current file operation|
|`DC_WFX_SCRIPT_MULTIFILEOP`|stores the value of the current file operation, which in the case of multiple files may require additional script executions with the **list** parameter|
|`DC_TERMCMD_CLOSE`|stores a command template to call the terminal|
|`DC_TERMCMD_OPEN`|stores a command template to call the terminal and keep the window open|
|`ENV_WFX_SCRIPT_STR_`*|stores the localized text of the corresponding template from the language file|

# Additional files

In addition to the script file, the folder may contain additional files.

File starting with the name of the script and with the suffix `_readme.txt` or `_readme[LANG].txt` is needed to display information in the plugin properties dialog.

File starting with the name of the script and with the suffix `.langs` is needed for localization and contains text for substitution or setting environment variables.

## *.langs files
If the script outputs text to stdout containing a pattern starting with `WFX_SCRIPT_STR_`, the plugin will replace this with the value from the *.langs file. The template may contain capital letters, numbers, and underscores.

If the file contains a template starting with `ENV_WFX_SCRIPT_STR_`, then it will be set as an environment variable when the script is launched.  This way it will be possible to get localized text from the script, which can be used to translate special file names. `WFX_SCRIPT_NAME` allow to change display name for the script in filepanel.

The current language is determined by the LANG environment variable.  If the *.langs file has a suitable section and template, then the text will be taken from there, otherwise the text will be taken from the `Default` section.

Example:
```
[Default]
WFX_SCRIPT_NAME=git ls-tree
WFX_SCRIPT_STR_REPO=Path to git repo
ENV_WFX_SCRIPT_STR_ERR=failed to get reflog

[ru_RU]
WFX_SCRIPT_STR_REPO=Путь к репозиторию git
ENV_WFX_SCRIPT_STR_ERR=не удалось получить reflog
```

# Notes
In addition to the script properties dialog, you can deinitialize vfs or reset settings by pressing F8 on the script file.  When using debugging mode, in addition to scripts, a special file will appear containing some information about script launches.  You can clear the viewed information by pressing F8 on a special file.


