MultiArc
--------

*See details in [DC help file](http://doublecmd.github.io/doc/en/multiarc.html)*

Copy-paste to `multiarc.ini` and enable in *Options* > *Archivers*.<br>
Also some separate files can be found [here](multiarc) (use *Options* > *Archivers* > *Other...* > *Import*).

- [AppImage](#appimage)
- [Base64](#b64)
- [cmdTotal + Wine](#cmdtotal)
- [InstallShield](#unshield)
- [TC WCX Test + Wine](#wcxtest)
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
<a name="unshield"><h3>InstallShield</h3></a>
```ini
[InstallShield]
Archiver=$COMMANDER_PATH/scripts/unshield
Description=installshield
ID=49 53 63 28
IDPos=0
IDSeekRange=0
Start=^Cabinet
End=^--------
Format0=z+__n+
List=%P l %AQ
Extract=%P x %AQ
FormMode=6
```
Script [unshield](scripts/unshield)

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
[wcxtest]
Archiver=$COMMANDER_PATH/scripts/wcxtest
Description=TC WCX Test - https://totalcmd.net/plugring/WCXTest.html
Start=^---------
Format0=$z+$yyyy?tt?dd hh?mm?ss$aaaaaa$n+
List=%PQU -l %AQ
Extract=%PQU -x %AQ %LQU
FormMode=10
```
Script [wcxtest](scripts/wcxtest)

---
<a name="cmdtotal"><h3>cmdTotal + Wine</h3></a>
```ini
[cmdtotal]
Archiver=$COMMANDER_PATH/scripts/cmdtotal
Description=cmdTotal - http://totalcmd.net/plugring/cmdtotal.html
Format0=$?? n+
List=%PQU l %AQ
Extract=%PQU x %AQ %LQU
FormMode=2
```
Script [cmdtotal](scripts/cmdtotal)
