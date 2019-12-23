proclst
========
View a list of running processes.

![plug-preview](https://i.imgur.com/ULFer1a.png)

## Compatibility
- `doublecmd-gtk2`
- `doublecmd-qt4`
- `doublecmd-qt5`

## Notes

### command line

`<pid>` (ext column) or `self` - change dir to `/proc/<pid>/` (required dc >r9178)

### button examples
<details>
  <summary>tl;dr pid = `%e`</summary>

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{C04826D3-B5AB-46C0-8D6E-1FC0494FB874}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>strace</Hint>
    <Command>{!SHELL}</Command>
    <Params>sudo strace -p%e -s9999 -e write</Params>
  </Program>
</doublecmd>
```

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{AB6EA31F-078B-40E9-BBB3-BA826D9F357A}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>lsof</Hint>
    <Command>{!SHELL}</Command>
    <Params>lsof -p %e</Params>
  </Program>
</doublecmd>
```

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{33DF487A-582E-43BB-ABF5-E1E145B63770}</ID>
    <Icon>cm_view</Icon>
    <Hint>show stdout</Hint>
    <Command>{!DC-VIEWER}</Command>
    <Params>/proc/%e/fd/1</Params>
  </Program>
</doublecmd>
```

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{9712C8E8-0B7F-4E5A-ACB9-2B4737518BBD}</ID>
    <Icon>cm_view</Icon>
    <Hint>show stderr</Hint>
    <Command>{!DC-VIEWER}</Command>
    <Params>/proc/%e/fd/2</Params>
  </Program>
</doublecmd>
```
</details>

## Credits
- Icon: [papirus icon theme](https://github.com/PapirusDevelopmentTeam/papirus-icon-theme)
