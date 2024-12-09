MultiArc
--------

*See details in [DC help file](http://doublecmd.github.io/doc/en/multiarc.html)*

Copy-paste to `multiarc.ini` and enable in *Options* > *Archivers*.<br>
Also some separate files can be found [here](multiarc) (use *Options* > *Archivers* > *Other...* > *Import*).

- [AppImage](#appimage)
- [Base64](#b64)
- [cmdTotal + Wine](#cmdtotal)
- [TC WCX Test + Wine](#wcxtest)
- [UHARC + Wine](#uharc)
- [UUEncode](#uue)


---
<a name="appimage"><h3>AppImage</h3></a>
```ini
[AppImage]
Archiver=7z
Description=AppImage (Type 1 and 2, x32 or x64)
ID=7F 45 4C 46 01 01 01 00 41 49 01, 7F 45 4C 46 01 01 01 00 41 49 02, 7F 45 4C 46 02 01 01 00 41 49 01, 7F 45 4C 46 02 01 01 00 41 49 02
IDPos=0
Extension=appimage2
Start=^-------------------
End=^-------------------
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz pppppppppppp  n+
List=%P -r0 l {-p%W} %AQ
Extract=%P x -y {-p%W} {%S} %AQ @%LQU
ExtractWithoutPath=%P e -y {-p%W} {%S} %AQ @%LQU
Test=%P t -y {%S} %AQ @%LQU
PasswordQuery=Enter password
FormMode=8
```
`appimage2` is a fake extension, this is a small workaround to open AppImage files with Ctrl+PgDn only.

---
<a name="b64"><h3>Base64</h3></a>
```ini
[Base64]
Archiver=%COMMANDER_PATH%/scripts/base64uue
Description=Base64
Extension=b64
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz n+
List=%P -l %AQ
Extract=%P -d %AQ %FQ
Add=%P -eb %AQ %FQ
```
Script [base64uue](scripts/base64uue), [forum](https://doublecmd.sourceforge.io/forum/viewtopic.php?p=24877#p24877)

---
<a name="uue"><h3>UUEncode</h3></a>
```ini
[UUEncode]
Archiver=%COMMANDER_PATH%/scripts/base64uue
Description=UUEncode
Extension=uue
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz n+
List=%P -l %AQ
Extract=%P -d %AQ %FQ
Add=%P -eu %AQ %FQ
```
Script [base64uue](scripts/base64uue), [forum](https://doublecmd.sourceforge.io/forum/viewtopic.php?p=24877#p24877)

---
<a name="wcxtest"><h3>TC WCX Test + Wine</h3></a>
```ini
[MhtUnPack]
Archiver=$HOME/.local/share/doublecmd/scripts/multiarc/wcx/wcxtest
Description=wcxtest + MhtUnPack.wcx
Extension=mht,mhtml,msg,xxe
Start=^---------
Format0=zzzzzzzzz  yyyy tt dd hh mm ss  aaaaaa	n+
List=%P -l MhtUnPack.wcx %AQ
Extract=%P -x MhtUnPack.wcx %AQ .
ExtractWithoutPath=
Test=%P -t MhtUnPack.wcx %AQ
Flags=2
FallBackArchivers=$HOME/.local/share/doublecmd/scripts/multiarc/wcx/wcxtest,$COMMANDER_PATH/scripts/multiarc/wcx/wcxtest
```
Script [wcxtest](scripts/multiarc/wcx/wcxtest)

---
<a name="cmdtotal"><h3>cmdTotal + Wine</h3></a>
```ini
[TotalObserver]
Archiver=$HOME/.local/share/doublecmd/scripts/multiarc/wcx/cmdTotal
Description=cmdTotal + TotalObserver.wcx
Extension=cab,msi
Start=^Target directory:
Format0=???????n+
List=%P TotalObserver.wcx l %AQ
Extract=%P TotalObserver.wcx x %AQ .
ExtractWithoutPath=
Test=%P TotalObserver.wcx t %AQ
Flags=2
FormMode=2
FallBackArchivers=$HOME/.local/share/doublecmd/scripts/multiarc/wcx/cmdTotal,$COMMANDER_PATH/scripts/multiarc/wcx/cmdTotal
```

```ini
[wcres]
Archiver=$HOME/.local/share/doublecmd/scripts/multiarc/wcx/cmdTotal
Description=cmdTotal + wcres.wcx
Extension=exe,dll,icl
Start=^Target directory:
Format0=???????n+
List=%P wcres.wcx l %AQ
Extract=%P wcres.wcx x %AQ .
ExtractWithoutPath=
Test=%P TotalObserver.wcx t %AQ
Flags=2
FormMode=2
FallBackArchivers=$HOME/.local/share/doublecmd/scripts/multiarc/wcx/cmdTotal,$COMMANDER_PATH/scripts/multiarc/wcx/cmdTotal
```
Script [cmdtotal](scripts/multiarc/wcx/cmdtotal)

---
<a name="uharc"><h3>UHARC + Wine</h3></a>
```ini
[UHARC]
Archiver=$HOME/.local/share/doublecmd/scripts/multiarc/uharc/uharc
Description=UHARC 0.6b
ID=55 48 41 06
IDPos=0
IDSeekRange=0
Extension=uha
Start=^--------
End=^--------
Format0=n+
Format1=                         zzzzzzzzzzzzzzz  dd TTT yyyy  hh mm ss  aaaa  *
List=%P l -d2 -y {-pw%W} %AQ
Extract=%P x -y %AQ {-pw%W} @%LQ
ExtractWithoutPath=%P e -y %AQ {-pw%W} @%LQ
Test=%P t -y -idle{ %S} %AQ
Add=%P a -y {%S} -ed+ {-pw%W} %AQ @%LQ
FormMode=10
FallBackArchivers=$HOME/.local/share/doublecmd/scripts/multiarc/uharc/uharc,$COMMANDER_PATH/scripts/multiarc/uharc/uharc,%ProgramFiles%\UHARC\UHARC.EXE
```
Script [uharc](scripts/multiarc/uharc/uharc)
