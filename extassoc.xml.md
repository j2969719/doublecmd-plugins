# All files

```xml
    <FileType>
      <Name>all files</Name>
      <IconFile/>
      <ExtensionList>file</ExtensionList>
      <Actions>
        <Action>
          <Name>gksu gedit</Name>
          <Command>gksu</Command>
          <Params>gedit %p</Params>
        </Action>
        <Action>
          <Name>ldd</Name>
          <Command>{!VIEWER}</Command>
          <Params>&lt;?ldd %p?&gt;</Params>
        </Action>
      </Actions>
    </FileType>       
```

# All folders

```xml
    <FileType>
      <Name>all folders</Name>
      <IconFile/>
      <ExtensionList>folder</ExtensionList>
      <Actions>
        <Action>
          <Name>gksu doublecmd</Name>
          <Command>gksu</Command>
          <Params>doublecmd %p</Params>
        </Action>
        <Action>
          <Name>ncdu</Name>
          <Command>ncdu</Command>
          <Params>%t1</Params>
          <StartPath>%p</StartPath>
        </Action>
      </Actions>
    </FileType>
```    

# Script [fileinfo.sh](https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=2727)

```xml
    <FileType>
      <Name>FileInfo.sh</Name>
      <IconFile/>
      <ExtensionList>ISO|TORRENT|SO|MO|DEB|TAR|LHA|ARJ|CAB|HA|RAR|ALZ|CPIO|7Z|ACE|ARC|ZIP|ZOO|PS|PDF|ODT|DOC|XLS|DVI|DJVU|EPUB|HTML|HTM|EXE|DLL|GZ|BZ2|XZ|MSI|RTF|FB2</ExtensionList>
      <Actions>
        <Action>
          <Name>fileinfo.sh</Name>
          <Command>{!VIEWER}</Command>
          <Params>&lt;?$COMMANDER_PATH/scripts/fileinfo.sh %p?&gt;</Params>
        </Action>
      </Actions>
    </FileType>
```
