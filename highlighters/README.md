Some additional files for syntax highlighting in the DC internal editor
-----------------------------------------------------------------------

This files are usual XML-based files, you can open them in text editor and read/change. In the beginning you can see tags `<Info>`, `<General>`, `<Author>` and `<Version>`. `<General>` has some attributes:

- *Name* : name for *Syntax highlight* menu. (I usually add an asterisk `*` to the end of this name as simple additional file indicator.)
- *Extensions* : list of supported file extensions.

`<Author>` has *Remark* for additional info.


## How to use

1. Portable version

Copy (or move) HGL-file(s) to the *highlighters* folder near `doublecmd.exe` and restart DC.

2. Installed version

Go to *Configuration* in main menu > *Options...* > *Configuration* > *Directories* > *Highlight:*, here you see list of possible directories.
Don't use `/usr/share/doublecmd/highlighters`, `C:\Program Files\Double Commander\highlighters` or other system directory: this requires root/admin rights, also this would be a intervention to the packages manager's work on Linux or DC installer for Windows. DC usually suggests the following additional path (besides the *highlighters* folder near `doublecmd.exe`):

- Windows XP: `C:\Documents and Settings\<UserName>\Local Settings\doublecmd\highlighters`
- Windows Vista/7-10: `C:\Users\<UserName>\AppData\Local\doublecmd\highlighters`
- Unix-like OS: `/home/<UserName>/.local/share/doublecmd/highlighters`

If the directory doesn't exist, create it. Now restart DC.

## How to create or edit

[HglEditor](http://totalcmd.net/plugring/HglEditor.html): create and use the *highlighters* folder near `HglEditor.exe`. HglEditor for Windows only, but you can use Wine.
