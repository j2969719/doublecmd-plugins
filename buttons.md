several buttons for the toolbar (copy-paste on the toolbar)

# ./configure
```
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
# make
```
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
# makepkg
```
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
# ldd
```
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
# pacman

```
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

```
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

```
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

```
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
# patch
```
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
# iso
```
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
# journalctl
```
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
# gvfs admin
```
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{CD73FAFB-6562-4DAB-AE3D-3AACA09114E9}</ID>
    <Icon>cm_changedir</Icon>
    <Hint>admin:///</Hint>
    <Command>cm_ChangeDir</Command>
    <Params>admin://</Params>
  </Program>
</doublecmd>
```
# gvfs webdav
```
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
# trash
```
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{CD73FAFB-6562-4DAB-AE3D-3AACA09114E9}</ID>
    <Icon>user-trash-symbolic</Icon>
    <Hint>trash:///</Hint>
    <Command>cm_ChangeDir</Command>
    <Params>trash://</Params>
  </Program>
</doublecmd>
```

```
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{F2ECCCAD-76BF-40D7-8CA5-F97DD97E62F1}</ID>
    <Icon>trashcan_empty</Icon>
    <Hint>Очистить корзину</Hint>
    <Command>sh</Command>
    <Params>-c "if (zenity --question --text=\"Empty trash?\") then (echo \"1\" ; sleep 1; gio trash --empty; echo \"99\" ; sleep 1) | (zenity --progress --auto-close --title=\"Processing\" --percentage=1) fi"</Params>
  </Program>
</doublecmd>
```
# man
```
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{EF436708-A740-4F00-8DDF-42477DC39E4B}</ID>
    <Icon>help</Icon>
    <Hint>manual</Hint>
    <Command>man</Command>
    <Params>%t1 %[Command;%f]</Params>
  </Program>
</doublecmd>
```
# systemd-analyze plot
via `ristretto`
```
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{87A98465-2212-4757-8043-7B4BE2C5B4CF}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>system boot-up</Hint>
    <Command>sh</Command>
    <Params>-c "systemd-analyze plot &gt; /tmp/_0curboot.svg &amp; ristretto /tmp/_0curboot.svg"</Params>
  </Program>
</doublecmd>
```
# ncdu
```
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{AD55AC39-CC71-48D3-97EB-EAF2AEB5355C}</ID>
    <Icon>baobab</Icon>
    <Hint>disk usage</Hint>
    <Command>ncdu</Command>
    <Params>%t1 %p</Params>
  </Program>
</doublecmd>
```
# duplicates
```
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{BAF5B30F-C8F9-4057-B7A6-67BE0B0D6C05}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>duplicates</Hint>
    <Command>find</Command>
    <Params>%d %t1 -type f -exec sha1sum '{}' ';' | sort | uniq --all-repeated=separate -w 33 | cut -c 35-</Params>
  </Program>
</doublecmd>
```

```
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Program>
    <ID>{F6F436E3-8F83-4867-A54D-B29CB9C04448}</ID>
    <Icon>cm_runterm</Icon>
    <Hint>duplicates (save to file)</Hint>
    <Command>sh</Command>
    <Params>-c "find %d -type f -exec sha1sum '{}' ';' | sort | uniq --all-repeated=separate -w 33 | cut -c 35- &gt; %[Result:;%dt/output.txt]"</Params>
  </Program>
</doublecmd>
```

# gio unmount
```
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