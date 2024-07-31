Several buttons for the toolbar
-------------------------------

*See details in [DC help file](http://doublecmd.github.io/doc/en/toolbar.html)*

Copy-paste on the toolbar.

### ./configure
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{9A1E9831-AA4B-477B-AABF-881CB40AB928}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>configure</Hint>
    <Command>./configure</Command>
    <Params>%t1</Params>
  </Program>
</doublecmd>
```

### make
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{59F4D17B-5E51-45FD-8C3C-B47518B6A79F}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>make</Hint>
    <Command>make</Command>
    <Params>%t1</Params>
    <StartPath>%D</StartPath>
  </Program>
</doublecmd>
```

### makepkg
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{3952BB0C-9A7C-42B6-843E-0F044AA482E7}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>makepkg -srf</Hint>
    <Command>makepkg</Command>
    <Params>%t1 -srf</Params>
  </Program>
</doublecmd>
```

### ldd
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{8851BEC2-EC1D-4078-BCBE-0FA423749ECD}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>ldd</Hint>
    <Command>file</Command>
    <Params>%t1 %f &amp; ldd -r -v %f</Params>
    <StartPath>%D</StartPath>
  </Program>
</doublecmd>
```

### pacman
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{7142430F-7B87-4F57-8C0C-9BA140ACFB34}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>install</Hint>
    <Command>sudo</Command>
    <Params>%t1 pacman -U %f</Params>
    <StartPath>%D</StartPath>
  </Program>
</doublecmd>
```

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{03D4FADD-961A-407C-A93B-2C2B2BCFC24E}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>which remote package a file belongs to</Hint>
    <Command>sudo</Command>
    <Params>%t1 pacman -Fs  %[Файл;%f]</Params>
  </Program>
</doublecmd>
```

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{C8B0427B-CDF1-42B4-9633-8C770E740297}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>which package a file belongs to</Hint>
    <Command>sudo</Command>
    <Params>%t1 pacman -Qo %f</Params>
  </Program>
</doublecmd>
```

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{C4C44036-E416-48C4-9014-909202E13476}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>orphans</Hint>
    <Command>sudo</Command>
    <Params>%t1 pacman -Qdt</Params>
  </Program>
</doublecmd>
```

### patch
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Menu>
    <ID>{C6601B8F-2BDD-4C36-8DE6-6F9961AA2416}</ID>
    <Icon>cm_comparecontents</Icon>
    <Hint>patch</Hint>
    <MenuItems>
      <Program>
        <ID>{71D615DC-1D3E-4C6C-ADF8-C63F4439B568}</ID>
        <Icon>cm_runterm</Icon>
        <Hint>patch (p0)</Hint>
        <Command>patch</Command>
        <Params>%t1 -Np0 -i %f</Params>
        <StartPath>%D</StartPath>
      </Program>
      <Program>
        <ID>{B1CC4A23-8581-46C4-8A72-C7511CB46EF0}</ID>
        <Icon>cm_runterm</Icon>
        <Hint>patch (p1)</Hint>
        <Command>patch</Command>
        <Params>%t1 -Np1 -i %f</Params>
        <StartPath>%D</StartPath>
      </Program>
      <Program>
        <ID>{38B86B6A-1352-41D3-8DAD-027362D02E6E}</ID>
        <Icon>cm_runterm</Icon>
        <Hint>create a patch</Hint>
        <Command>diff</Command>
        <Params>%t1 -Naur %ds %dt &gt; patch.diff</Params>
        <StartPath>%D</StartPath>
      </Program>
      <Program>
        <ID>{C9B122CA-7BDA-4FDA-8D43-B8F036C5FAA7}</ID>
        <Icon>cm_runterm</Icon>
        <Hint>create a patch(active panel)</Hint>
        <Command>diff</Command>
        <Params>%t1 -Naur %fs &gt; patch.diff</Params>
      </Program>
    </MenuItems>
  </Menu>
</doublecmd>
```

### iso
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Menu>
    <ID>{B169885A-2842-4309-B952-2AB9EC8A07AA}</ID>
    <Icon>media-cdrom</Icon>
    <Hint>ISO</Hint>
    <MenuItems>
      <Program>
        <ID>{5FFA6565-B95A-44F4-A22D-B0EEE6CE43C9}</ID>
        <Icon>media-cdrom</Icon>
        <Hint>make iso</Hint>
        <Command>mkisofs</Command>
        <Params>%t1 -r -J -o %Dt/temp.iso %Ds</Params>
      </Program>
      <Program>
        <ID>{4869E017-0F1C-46B5-AA3D-03ACE6EABBF1}</ID>
        <Icon>media-cdrom</Icon>
        <Hint>dd (if=/dev/sr0)</Hint>
        <Command>dd</Command>
        <Params>%t1 if=/dev/sr0 of=%Ds/temp.iso</Params>
      </Program>
      <Program>
        <ID>{C4A23D57-2D84-449D-B60F-9349D98EF61B}</ID>
        <Icon>media-cdrom</Icon>
        <Hint>dd (if=/dev/sr0)</Hint>
        <Command>dd</Command>
        <Params>%t1 if=/dev/sr1 of=%Ds/temp.iso</Params>
      </Program>
      <Program>
        <ID>{C9F2F072-C9A9-4AC9-BA65-DCE9FCCEA122}</ID>
        <Icon>media-removable</Icon>
        <Hint>dd</Hint>
        <Command>sudo</Command>
        <Params>%t1 dd if=%p of=%[Path;/dev/sdb] status=progress</Params>
      </Program>
      <Program>
        <ID>{840F4382-F367-42F7-B811-A7DDA7326E54}</ID>
        <Icon>media-removable</Icon>
        <Hint>dd bs=4M</Hint>
        <Command>sudo</Command>
        <Params>%t1 dd bs=4M if=%p of=%[Path;/dev/sdb] status=progress</Params>
      </Program>
      <Program>
        <ID>{EFC05523-4B96-4A9D-8E35-7726928B51C6}</ID>
        <Icon>media-removable</Icon>
        <Hint>sync</Hint>
        <Command>sync</Command>
        <Params>%t1 </Params>
      </Program>
    </MenuItems>
  </Menu>
</doublecmd>
```

### journalctl
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Menu>
    <ID>{FEA787A5-376B-4928-9EC7-39D20F8D439F}</ID>
    <Icon>logviewer</Icon>
    <Hint>journal</Hint>
    <MenuItems>
      <Program>
        <ID>{347E8F60-EDE2-4D54-B72A-E497867B575B}</ID>
        <Icon>logviewer</Icon>
        <Hint>boot</Hint>
        <Command>journalctl</Command>
        <Params>%t1 -b -0</Params>
      </Program>
      <Program>
        <ID>{CD02E8D6-264B-4B4B-89A7-0475360A82A9}</ID>
        <Icon>logviewer</Icon>
        <Hint>prev boot</Hint>
        <Command>journalctl</Command>
        <Params>%t1 -b -1</Params>
      </Program>
      <Program>
        <ID>{49243381-C491-4DA2-9116-94782D429D68}</ID>
        <Icon>logviewer</Icon>
        <Hint>second prev boot</Hint>
        <Command>journalctl</Command>
        <Params>%t1 -b -2</Params>
      </Program>
      <Program>
        <ID>{789B9422-B4AA-48FF-AA0A-B75F1E9B3A6A}</ID>
        <Icon>logviewer</Icon>
        <Hint>critical</Hint>
        <Command>journalctl</Command>
        <Params>%t1 -p err..alert -e</Params>
      </Program>
      <Program>
        <ID>{77D92DCA-E18B-402C-A736-AADBA6C2EC8F}</ID>
        <Icon>logviewer</Icon>
        <Hint>follow</Hint>
        <Command>journalctl</Command>
        <Params>%t1 -f</Params>
      </Program>
    </MenuItems>
  </Menu>
</doublecmd>
```

### gvfs admin
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{CD73FAFB-6562-4DAB-AE3D-3AACA09114E9}</ID>
    <Icon>cm_changedir</Icon>
    <Hint>admin:///</Hint>
    <Command>cm_ChangeDir</Command>
    <Params>admin:///</Params>
  </Program>
</doublecmd>
```

### gvfs webdav
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Menu>
    <ID>{E3666716-B848-4EBB-9B37-655E6DCCF15A}</ID>
    <Icon>$COMMANDER_PATH/pixmaps/yadisk.png</Icon>
    <Hint>Yandex.Disk</Hint>
    <MenuItems>
      <Command>
        <ID>{4E1E5E82-60F3-403A-8CEF-5090421D9505}</ID>
        <Icon>$COMMANDER_PATH/pixmaps/yadisk.png</Icon>
        <Hint>connect</Hint>
        <Command>cm_ChangeDir</Command>
        <Param>davs://webdav.yandex.ru</Param>
      </Command>
      <Program>
        <ID>{E18D6F35-378E-4438-BB60-4DEAA4FF48E1}</ID>
        <Icon>network-wired-disconnected</Icon>
        <Hint>disconnect</Hint>
        <Command>sh</Command>
        <Params>-c "gio mount -u davs://webdav.yandex.ru/"</Params>
      </Program>
    </MenuItems>
  </Menu>
</doublecmd>
```

### Trash
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{CD73FAFB-6562-4DAB-AE3D-3AACA09114E9}</ID>
    <Icon>user-trash-symbolic</Icon>
    <Hint>trash:///</Hint>
    <Command>cm_ChangeDir</Command>
    <Params>trash:///</Params>
  </Program>
</doublecmd>
```

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{F2ECCCAD-76BF-40D7-8CA5-F97DD97E62F1}</ID>
    <Icon>trashcan_empty</Icon>
    <Hint>Empty trash</Hint>
    <Command>sh</Command>
    <Params>-c "if (zenity --question --text=\"Empty trash?\") then (echo \"1\" ; sleep 1; gio trash --empty; echo \"99\" ; sleep 1) | (zenity --progress --auto-close --title=\"Processing\" --percentage=1) fi"</Params>
  </Program>
</doublecmd>
```

### man
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{EF436708-A740-4F00-8DDF-42477DC39E4B}</ID>
    <Icon>help</Icon>
    <Hint>Manual</Hint>
    <Command>man</Command>
    <Params>%t1 %[Command;%f]</Params>
  </Program>
</doublecmd>
```

### systemd-analyze plot
via `ristretto`
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{87A98465-2212-4757-8043-7B4BE2C5B4CF}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>System boot-up</Hint>
    <Command>sh</Command>
    <Params>-c "systemd-analyze plot &gt; /tmp/_0curboot.svg &amp;&amp; ristretto /tmp/_0curboot.svg"</Params>
  </Program>
</doublecmd>
```

### Disk usage (ncdu)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{AD55AC39-CC71-48D3-97EB-EAF2AEB5355C}</ID>
    <Icon>baobab</Icon>
    <Hint>Disk usage</Hint>
    <Command>ncdu</Command>
    <Params>%t1 %p</Params>
  </Program>
</doublecmd>
```

### Duplicates
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{BAF5B30F-C8F9-4057-B7A6-67BE0B0D6C05}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>Duplicates</Hint>
    <Command>find</Command>
    <Params>%"0 %t1 "%D" -type f -exec sha1sum "{}" ";" | sort | uniq --all-repeated=separate -w 40 | cut -c 43-</Params>
  </Program>
</doublecmd>
```

```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{F6F436E3-8F83-4867-A54D-B29CB9C04448}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>Duplicates (save to file)</Hint>
    <Command>sh</Command>
    <Params>-c %"0 "find '%D' -type f -exec sha1sum '{}' ';' | sort | uniq --all-repeated=separate -w 40 | cut -c 43- &gt; '%[Result:;%dt/output.txt]'"</Params>
  </Program>
</doublecmd>
```

### Find samefile (hardlink)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{AD4D866A-9B86-4193-9DA4-9D78C8BEF66B}</ID>
    <Icon>cm_search</Icon>
    <Hint>Find samefile (hardlink)</Hint>
    <Command>find</Command>
    <Params>%"0 "%[path;%D]" -samefile "%p0" %t1</Params>
  </Program>
</doublecmd>
```

### gio unmount
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{A4A25414-E759-40DE-9341-0533C187E2BF}</ID>
    <Icon>player_eject</Icon>
    <Hint>Unmount</Hint>
    <Command>sh</Command>
    <Params>-c 'gio mount -l | grep "Mount(" | sed "s/^.*\s-&gt;\s//g" | zenity --list --title="Unmount" --column="Mount" | xargs gio mount -u'</Params>
  </Program>
</doublecmd>
```

### Git
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Menu>
    <ID>{EF6CA5CA-69F1-459D-8727-849414E1FF85}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>Git</Hint>
    <MenuItems>
      <Program>
        <ID>{A3541377-4F91-45E6-A8ED-DC6FA7D588C5}</ID>
        <Icon>cm_gotolastfile</Icon>
        <Hint>Pull</Hint>
        <Command>git</Command>
        <Params>pull %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{3848EB14-8E03-492E-8486-02B76BDB5538}</ID>
        <Icon>cm_operationsviewer</Icon>
        <Hint>Status</Hint>
        <Command>git</Command>
        <Params>status %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{9E5BF7CA-31E3-412B-AA92-4D31E241AD36}</ID>
        <Icon>cm_comparecontents</Icon>
        <Hint>View diff (current file, active panel)</Hint>
        <Command>git</Command>
        <Params>%t1 diff %fs &gt; patch.diff</Params>
      </Program>
      <Program>
        <ID>{23A821C3-A607-4B59-AF86-F4739592413E}</ID>
        <Icon>cm_markmarkall</Icon>
        <Hint>Add all</Hint>
        <Command>git</Command>
        <Params>add * %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{19316A20-1822-43EE-87C1-827BE3928FA2}</ID>
        <Icon>cm_markmarkall</Icon>
        <Hint>Add file</Hint>
        <Command>git</Command>
        <Params>add %p %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{EF71220F-8746-413B-A822-1C40216C284B}</ID>
        <Icon>cm_renameonly</Icon>
        <Hint>Commit</Hint>
        <Command>git</Command>
        <Params>commit -m '%[Description;]' %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{7D6E9C67-9661-4904-A212-80E17B1441F8}</ID>
        <Icon>cm_gotolastfile</Icon>
        <Hint>Fetch</Hint>
        <Command>git</Command>
        <Params>fetch %[Target;origin] %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{4947C23B-7EB6-46E5-840A-981A1E664979}</ID>
        <Icon>cm_gotofirstfile</Icon>
        <Hint>Push</Hint>
        <Command>git</Command>
        <Params>push %[Target;origin HEAD] %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{3521972F-963D-4B4A-B110-58224C3DCC01}</ID>
        <Icon>cm_copy</Icon>
        <Hint>Clone</Hint>
        <Command>git</Command>
        <Params>clone %[Target;] %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{3521972F-963D-4B4A-B110-58224C3DCC01}</ID>
        <Icon>cm_copy</Icon>
        <Hint>Clone (recursive)</Hint>
        <Command>git</Command>
        <Params>clone --recursive %[Target;] %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
      <Program>
        <ID>{A45C14BD-5794-43A7-B194-28864F1BDA1A}</ID>
        <Icon>cm_gotofirstfile</Icon>
        <Hint>Remote</Hint>
        <Command>git</Command>
        <Params>remote %t1</Params>
        <StartPath>%d</StartPath>
      </Program>
    </MenuItems>
  </Menu>
</doublecmd>
```

### Force internal viewer (for assigning hotkey)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{81124796-B4A5-453C-BE0E-3F846C8C9A5E}</ID>
    <Icon>cm_view</Icon>
    <Hint>Internal viewer</Hint>
    <Command>{!DC-VIEWER}</Command>
    <Params>%p</Params>
  </Program>
</doublecmd>
```

### Emblems
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Menu>
    <ID>{97CE02F0-3471-4132-8F88-8A81CE93209A}</ID>
    <Icon>emblem-default</Icon>
    <Hint>Emblems</Hint>
    <MenuItems>
      <Program>
        <ID>{0169607F-0833-4C5C-992E-30A135C7830F}</ID>
        <Icon>edit-cut</Icon>
        <Hint>Clear</Hint>
        <Command>gio</Command>
        <Params>set -t unset %p metadata::emblems</Params>
      </Program>
      <Program>
        <ID>{76DFF65F-6D47-4A8C-8672-B2E79A3CAD7A}</ID>
        <Icon>emblem-default</Icon>
        <Hint>emblem-default</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-default</Params>
      </Program>
      <Program>
        <ID>{567DE734-05B9-4803-A6D2-E1E3F0F9D9AC}</ID>
        <Icon>emblem-documents</Icon>
        <Hint>emblem-documents</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-documents</Params>
      </Program>
      <Program>
        <ID>{3317A393-75F7-460D-8733-0124CEC792A5}</ID>
        <Icon>emblem-downloads</Icon>
        <Hint>emblem-downloads</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-downloads</Params>
      </Program>
      <Program>
        <ID>{B43627E9-DCC3-47DD-A954-C73EF9B3DEFF}</ID>
        <Icon>emblem-favorite</Icon>
        <Hint>emblem-favorite</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-favorite</Params>
      </Program>
      <Program>
        <ID>{A9773A7C-365A-49AC-8AAE-63001BF39C45}</ID>
        <Icon>emblem-important</Icon>
        <Hint>emblem-important</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-important</Params>
      </Program>
      <Program>
        <ID>{62DC9DFC-C701-425B-95F3-046D0E9C80B3}</ID>
        <Icon>emblem-mail</Icon>
        <Hint>emblem-mail</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-mail</Params>
      </Program>
      <Program>
        <ID>{ED0A9F83-84BE-45D7-BD22-1765C6DF5B40}</ID>
        <Icon>emblem-new</Icon>
        <Hint>emblem-new</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-new</Params>
      </Program>
      <Program>
        <ID>{763528E5-A9CB-4D0C-8204-96BB78F2981F}</ID>
        <Icon>emblem-package</Icon>
        <Hint>emblem-package</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-package</Params>
      </Program>
      <Program>
        <ID>{E854A8F3-1AA0-4B1D-B17F-411BF1176FBF}</ID>
        <Icon>emblem-photos</Icon>
        <Hint>emblem-photos</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-photos</Params>
      </Program>
      <Program>
        <ID>{57EB96A8-9D7B-484A-83E7-ACB21FFB0EB5}</ID>
        <Icon>emblem-readonly</Icon>
        <Hint>emblem-readonly</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-readonly</Params>
      </Program>
      <Program>
        <ID>{E0D0E7DD-71F5-461A-B49D-0D8548D98354}</ID>
        <Icon>emblem-shared</Icon>
        <Hint>emblem-shared</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-shared</Params>
      </Program>
      <Program>
        <ID>{67DB89EC-A9CC-43C4-BFAF-E3715F1F55F4}</ID>
        <Icon>emblem-synchronizing</Icon>
        <Hint>emblem-synchronizing</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-synchronizing</Params>
      </Program>
      <Program>
        <ID>{A404DD57-F0F2-43D5-BC81-F89D53536CBD}</ID>
        <Icon>emblem-system</Icon>
        <Hint>emblem-system</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-system</Params>
      </Program>
      <Program>
        <ID>{9B496328-CBE1-4568-9D3B-6A4DC150D8BE}</ID>
        <Icon>emblem-unreadable</Icon>
        <Hint>emblem-unreadable</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-unreadable</Params>
      </Program>
      <Program>
        <ID>{EF52C175-80DE-44BC-8332-1992EE88A07A}</ID>
        <Icon>emblem-urgent</Icon>
        <Hint>emblem-urgent</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-urgent</Params>
      </Program>
      <Program>
        <ID>{B1090949-EE57-45F9-933D-C3CB02A64311}</ID>
        <Icon>emblem-web</Icon>
        <Hint>emblem-web</Hint>
        <Command>gio</Command>
        <Params>set -t stringv %p metadata::emblems emblem-web</Params>
      </Program>
    </MenuItems>
  </Menu>
</doublecmd>
```

### Convert images
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{138CE458-F38D-439F-AFAF-1E3623E8E13B}</ID>
    <Icon>camera-photo</Icon>
    <Hint>Convert images</Hint>
    <Command>mogrify</Command>
    <Params>%[mogrify options; -format jpg *.%e1]</Params>
    <StartPath>%D</StartPath>
  </Program>
</doublecmd>
```

### Show file properties window (default filemanger)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{C31D381F-2B47-4F19-8D0B-C797FD6857E2}</ID>
    <Icon>cm_fileproperties</Icon>
    <Hint>File Properties...</Hint>
    <Command>dbus-send</Command>
    <Params>%"0 --dest=org.freedesktop.FileManager1 --type=method_call /org/freedesktop/FileManager1 org.freedesktop.FileManager1.ShowItemProperties array:string:%p0{"file:}{",} string:"0"</Params>
  </Program>
</doublecmd>
```

### kio fuse
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{8959646D-CFE9-43FB-BA8A-590C577FE914}</ID>
    <Icon>network-server</Icon>
    <Hint>kio fuse</Hint>
    <Command>{!SHELL}</Command>
    <Params>dbus-send --session --print-reply --type=method_call --dest=org.kde.KIOFuse /org/kde/KIOFuse org.kde.KIOFuse.VFS.mountUrl string:%[uri;ftp://user:password@server/directory]</Params>
  </Program>
</doublecmd>
```

### unmount dir
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{0EDE249B-2C27-4554-9250-79D486B2AB53}</ID>
    <Icon>media-eject</Icon>
    <Hint>unmount dir</Hint>
    <Command>{!SHELL}</Command>
    <Params>fusermount3  -u %D</Params>
  </Program>
</doublecmd>
```

## Scripts

You can change the script path, icon and description.

### Edit symlink
[editsymlink.lua](scripts/lua/editsymlink.lua)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{FE3433C9-3980-441C-8D08-CE0FEC98F740}</ID>
    <Icon>cm_symlink</Icon>
    <Hint>Edit Symlink</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$COMMANDER_PATH/scripts/lua/editsymlink.lua</Param>
    <Param>%"0%p0</Param>
  </Command>
</doublecmd>
```


### Go to link target
[gotolinktarget.lua](scripts/lua/gotolinktarget.lua)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{4651FC8B-CC8A-4D8B-8741-DB29D812B7E3}</ID>
    <Icon>cm_symlink</Icon>
    <Hint>Go to link target</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$COMMANDER_PATH/scripts/lua/gotolinktarget.lua</Param>
    <Param>%"0%p0</Param>
    <Param>onlydir</Param>
  </Command>
</doublecmd>
```


### Mark all or unmark all *(cross platform)*
[markunmark.lua](scripts/lua/markunmark.lua)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{58430566-6032-4BCC-986E-FE206ADA40FF}</ID>
    <Icon>cm_markmarkall</Icon>
    <Hint>MarkUnmark</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/markunmark.lua</Param>
  </Command>
</doublecmd>
```


### Move tab to inactive panel *(cross platform)*
[quasimovetab.lua](scripts/lua/quasimovetab.lua)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{5CF4F955-29F7-41B1-AD5C-6B991A099DFC}</ID>
    <Icon>cm_newtab</Icon>
    <Hint>Move tab to opposite side</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$COMMANDER_PATH/scripts/lua/quasimovetab.lua</Param>
    <Param>%D</Param>
  </Command>
</doublecmd>
```


### Create new file *(cross platform)*
[CreateNewFile.lua](scripts/lua/CreateNewFile/CreateNewFile.lua), [templates](scripts/lua/CreateNewFile/newfiles)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{78AE07D0-42A0-4238-92BB-BE802AFB6F48}</ID>
    <Icon>cm_editnew</Icon>
    <Hint>Create new file</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/CreateNewFile.lua</Param>
    <Param>%"0%D</Param>
    <Param>%COMMANDER_PATH%/scripts/lua/newfiles</Param>
  </Command>
</doublecmd>
```


### Make dir(s) with some additional features *(cross platform)*
[MakeDir.lua](scripts/lua/MakeDir.lua), see details in the beginning of script.
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{78AE07D0-42A0-4238-92BB-BE802AFB6F49}</ID>
    <Icon>cm_makedir</Icon>
    <Hint>Make dir(s)</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/MakeDir.lua</Param>
    <Param>%"0%D</Param>
  </Command>
</doublecmd>
```


### Calculate or verify checksum, auto choose *(cross platform)*
[CheckSum.lua](scripts/lua/CheckSum.lua)
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="utf-8"?>
<doublecmd>
  <Command>
    <ID>{AA396254-DDBD-4CD9-8C51-48FDDFA4F2C3}</ID>
    <Icon>cm_checksumverify</Icon>
    <Hint>Calculate or verify checksum (auto choose)</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/CheckSum.lua</Param>
    <Param>%"0%es</Param>
    <Param>%"0%ps2</Param>
  </Command>
</doublecmd>
```


### Set custom extended attribute(s)
[SetFAttrCustom.lua](scripts/lua/SetFAttrCustom.lua). Also can be useful wdx-plugin: [getfattrcustomwdx.lua](plugins/wdx/scripts/getfattrcustomwdx.lua).
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{78AE07D0-42A0-4238-92BB-BE802AFB6F48}</ID>
    <Icon>cm_executescript</Icon>
    <Hint>Set custom extended attributes</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/SetFAttrCustom.lua</Param>
    <Param>%LU</Param>
  </Command>
</doublecmd>
```


### Save clipboard contents to text file *(cross platform)*
[SaveClipboardToFile.lua](scripts/lua/SaveClipboardToFile.lua). Also you can open this new file in viewer or editor immediately, see details in the beginning of script.
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{98AEAAD0-42A0-4238-92BB-BE80744B6F48}</ID>
    <Icon>cm_executescript</Icon>
    <Hint>Save clipboard contents to text file</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/SaveClipboardToFile.lua</Param>
    <Param>%"0%Ds</Param>
  </Command>
</doublecmd>
```


### Quick filter with a predefined list of masks *(cross platform)*
[QuickFilter.lua](scripts/lua/QuickFilter.lua).
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{98AEAAD0-1111-4238-92BB-BE807411B128}</ID>
    <Icon>cm_executescript</Icon>
    <Hint>Quick filter with a predefined list of masks</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/QuickFilter.lua</Param>
  </Command>
</doublecmd>
```


### cm_FlatView/cm_FlatViewSel: copy files with keeping directory structure
[CopyTree.py](scripts/CopyTree.py). Use with cm_FlatView/cm_FlatViewSel: copy selected files to target directory with keeping source directory structure.
```xml
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{28AE07D0-12A0-4238-92BB-BE815AFB6F48}</ID>
    <Icon>cm_copy</Icon>
    <Hint>cm_FlatView/cm_FlatViewSel: copy files with keeping directory structure</Hint>
    <Command>%COMMANDER_PATH%/scripts/CopyTree.py</Command>
    <Param> %t1 %"0 "%LU" "%Ds" "%Dt"</Param>
  </Program>
</doublecmd>
```


### Marker *(cross platform)*
For file highlighting like in *Colors* > *File types*, but "on the fly": add selected color, delete or change. So you can use this scripts for file tagging. See details [here](scripts/lua/marker).


### Tagging *(cross platform)*
Assign any tag to any file or folder (local file systems). See details [here](scripts/lua/tags).
