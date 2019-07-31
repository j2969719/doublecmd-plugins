File associations and actions
=============================

### All files

```xml
    <FileType>
      <Name>All files</Name>
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

### All folders

```xml
    <FileType>
      <Name>All folders</Name>
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

### Script [fileinfo.sh](https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=2727)

```xml
    <FileType>
      <Name>FileInfo.sh</Name>
      <IconFile/>
      <ExtensionList>iso|torrent|so|mo|deb|tar|lha|arj|cab|ha|rar|alz|cpio|7z|ace|arc|zip|zoo|ps|pdf|odt|doc|xls|dvi|djvu|epub|html|htm|exe|dll|gz|bz2|xz|msi|rtf|fb2</ExtensionList>
      <Actions>
        <Action>
          <Name>fileinfo.sh</Name>
          <Command>{!VIEWER}</Command>
          <Params>&lt;?$COMMANDER_PATH/scripts/fileinfo.sh %p?&gt;</Params>
        </Action>
      </Actions>
    </FileType>
```

### Plugins
```xml
    <FileType>
      <Name>Plugins</Name>
      <IconFile>cm_configplugins</IconFile>
      <ExtensionList>wcx|wlx|wfx|wdx|wdx64|wfx64|wlx64|wcx64</ExtensionList>
      <Actions>
        <Action>
          <Name>Add Plugin</Name>
          <Command>cm_AddPlugin</Command>
          <Params>%p</Params>
        </Action>
      </Actions>
    </FileType>
```

### Checksum (all supported DC)
```xml
    <FileType>
      <Name>CheckSum</Name>
      <IconFile>cm_checksumcalc</IconFile>
      <ExtensionList>blake2b|blake2bp|tiger|sha512|sha384|sha256|sha224|sha3|sha|sfv|ripemd160|ripemd128|blake2s|blake2sp|md5|md4|haval|crc32</ExtensionList>
      <Actions>
        <Action>
          <Name>open</Name>
          <Command>cm_CheckSumVerify</Command>
        </Action>
      </Actions>
    </FileType>
```
