MultiArc
--------

*See details in [DC help file](http://doublecmd.github.io/doc/en/multiarc.html)*

Copy-paste to `multiarc.ini` and enable in *Options* > *Archivers*.<br>
Also some separate files can be found [here](multiarc) (use *Options* > *Archivers* > *Other...* > *Import*).

- [7Zip self-extracting archive](#7zsfx)
- [BALZ](#balz)
- [Base64](#b64)
- [CHM](#chm)
- [FreeArc](#freearc)
- [grpar](#grpar)
- [Inno Setup installer](#innosetup)
- [InstallShield](#unshield)
- [lrzip](#lrzip)
- [LZ4](#lz4)
- [lzop](#lzop)
- [mcm](#mcm)
- [Microsoft Cabinet](#cab)
- [Microsoft Windows Installer](#msi)
- [MS-DOS installation compression](#szdd)
- [Nullsoft Scriptable Install System](#nsis)
- [pakextract](#pakextract)
- [PAQ8](#paq8o9)
- [QUAD](#quad)
- [RAR + 7Zip](#rar)
- [The Unarchiver](#unar)
- [UNACE](#unace)
- [UPX](#upx)
- [UUEncode](#uue)
- [ZPAQ](#zpaq)
- [ZSTD](#zstd)

---
<a name="7zsfx"><h3>7Zip self-extracting archive</h3></a>
```ini
[7ZSfx]
Archiver=7z
Description=7-Zip - www.7-zip.org
ID=37 7A BC AF, 50 4B 03 04
IDPos=<SeekID>
Start=^-------------------
End=^-------------------
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz pppppppppppp  n+
List=%P -r0 l %AQA
Extract=%P x -y {-p%W} {%S} %AQA @%LQU
ExtractWithoutPath=%P e -y {-p%W} {%S} %AQA @%LQU
PasswordQuery=Enter password
FormMode=8
```

---
<a name="balz"><h3>BALZ</h3></a>
```ini
[BALZ]
Archiver=$COMMANDER_PATH/scripts/cutext
Description=BALZ
Extension=balz
Format0=n+
List=%P %AQ
Extract=balz d %AQ %FQ
Add=balz c{%S} %FQ %AQ
```
[BALZ](https://sourceforge.net/projects/balz/), script [cutext](scripts/cutext)

---
<a name="b64"><h3>Base64</h3></a>
```ini
[Base64]
Archiver=%COMMANDER_PATH%/utils/base64uue
Description=Base64
Extension=b64
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz n+
List=%P -l %AQ
Extract=%P -d %AQ %FQ
Add=%P -eb %AQ %FQ
```
Script [base64uue](utils/base64uue), [forum](https://doublecmd.sourceforge.io/forum/viewtopic.php?p=24877#p24877)

---
<a name="chm"><h3>CHM</h3></a>
```ini
[CHM]
Archiver=7z
Description=Compressed Help Module
ID=49 54 53 46
IDPos=0
Start=^-------------------
End=^-------------------
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz pppppppppppp  n+
List=%P -r0 l %AQA
Extract=%P x -y {-p%W} {%S} %AQA @%LQU
FormMode=8
```

---
<a name="freearc"><h3>FreeArc</h3></a>
Native Linux [binary](utils/freearc):
```ini
[FreeArc]
Archiver=arc
Description=FreeArc 0.666
ID=41 72 43 01
IDPos=0, -38, -39, -40, <SeekID>
Extension=arc
Start=^--
End=^--
Format0=yyyy tt dd hh mm ss aaaaaaa zzzzzzzzzzzzzzz ppppppppppppppp rrrrrrrr nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
List=%P v --noarcext -- %AQU
Extract=%P x {-p%W} -y -sclUTF8 -- %AQU @%LU
ExtractWithoutPath=%P e {-p%W} -y -sclUTF8 -- %AQU @%LU
Test=%P t --noarcext  %AQA
Delete=%P d --noarcext %AQA @%LU
Add=%P a {-p%W} {-ap%RQA} --noarcext {%S} -sclUTF8 -- %AQU @%LU
AddSelfExtract=%P a {-p%W} {-ap%RQA} -sfx --noarcext -sclANSI {%S} -- %AQA @%LA
PasswordQuery=Enter decryption password:
FormMode=8
```
With Wine:

```ini
[FreeArc(wine)]
Archiver=$COMMANDER_PATH/scripts/freearc
Description=FreeArc 0.67 (wine)
ID=41 72 43 01
IDPos=0, -38, -39, -40, <SeekID>
Extension=arc
Start=^--
End=^--
Format0=yyyy tt dd hh mm ss aaaaaaa zzzzzzzzzzzzzzz ppppppppppppppp rrrrrrrr nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
List=%P v --noarcext -- %AQU
Extract=%P x {-p%W} -y --noarcext -sclUTF8 -- %AQU @%LU
ExtractWithoutPath=%P e {-p%W} -y --noarcext -sclUTF8 -- %AQU @%LU
Test=%P t --noarcext -sclUTF8 -- %AQU
Delete=%P d --noarcext -sclUTF8 -- %AQU @%LU
Add=%P a {-p%W} {-ap%RQA} --noarcext -sclUTF8 {%S} -- %AQU @%LU
AddSelfExtract=%P a {-p%W} {-ap%RQA} -sfx --noarcext -sclUTF8 {%S} -- %AQU @%LU
PasswordQuery=Enter decryption password:
FormMode=10
```
Script [freearc](scripts/freearc)

---
<a name="grpar"><h3>grpar</h3></a>
```ini
[GRP]
Archiver=grpar
Description=DUKE3D GRP
ID=4B 65 6E 53 69 6C 76 65 72 6D 61 6E
IDPos=0
IDSeekRange=0
Extension=grp
Format0=n+ (z+
List=%P -t -v -f %AQ
Extract=%P -x -f %AQ %FQ
```
[grpar](http://github.com/martymac/grpar)

---
<a name="innosetup"><h3>Inno Setup installer</h3></a>
```ini
[InnoSetup(gog)]
Archiver=innoextract
Description=innoextract 1.7 (GOG)
ID=49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 35 2E
IDPos=<SeekID>
Format0=z+ n+
List=%P --list-sizes -g -s %AQU
Extract=%P -e -g -q  %AQU
ExtractWithoutPath=%P -e -g -q  %AQU -I %FQU
```
```ini
[InnoSetup]
Archiver=innoextract
Description=innoextract 1.7
ID=49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 35 2E, 49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 34 2E, 49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 33 2E, 49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 32 2E, 49 6E 6E 6F
IDPos=<SeekID>
Format0=z+ n+
List=%P --list-sizes -s %AQU
Extract=%P -e -q  %AQU
ExtractWithoutPath=%P -e -q  %AQU -I %FQU
```
```ini
[InnoSetup (unpack_one)]
Archiver=$COMMANDER_PATH/scripts/unpack_one
Description=innoextract 1.8 (script unpack_one)
ID=49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 35 2E, 49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 34 2E, 49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 33 2E, 49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 32 2E, 49 6E 6E 6F
IDPos=<SeekID>
Format0=z+$n+
List=innoextract --list-sizes -qsg %AQU
Extract=%PQU %FQU 'innoextract -esqg %AQU -I%FQU'
ExtractWithoutPath=innoextract -esqg  %AQU -I%FQU
Test=innoextract -tqg  %AQU
FormMode=2
```
Script [unpack_one](scripts/unpack_one)


With Wine:

```ini
[InnoSetup(wine)]
Archiver=$COMMANDER_PATH/scripts/innounp
Description=InnoSetup без -c%R           @%LQA
ID=49 6E 6E 6F 20 53 65 74 75 70 20 53 65 74 75 70 20 44 61 74 61 20 28 35 2E
IDPos=<SeekID>
Start=^--------------------------------------
End=^--------------------------------------
Format0=zzzzzzzzzz  yyyy.tt.dd hh:mm  nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
List=%P -v -m %AQA
Extract=%P -x -m %AQ
ExtractWithoutPath=%P -e -m %AQ %FQ
FormMode=2
```
Script [innounp](scripts/innounp)

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
List=%P l %AQA
Extract=%P x %AQA
FormMode=6
```
Script [unshield](scripts/unshield)

---
<a name="lrzip"><h3>lrzip</h3></a>
```ini
[lrzip]
Archiver=$COMMANDER_PATH/scripts/cutext
Description=lrzip
Extension=lrz
Format0=n+
List=%P %AQ
Extract=lrzip -d %AQ -o %FQ
Add=lrzip {%S} -f %FQ -o %AQ
```
[lrzip](https://github.com/ckolivas/lrzip), script [cutext](scripts/cutext)

---
<a name="lz4"><h3>LZ4</h3></a>
```ini
[LZ4]
Archiver=$COMMANDER_PATH/scripts/cutext
Description=LZ4
Extension=lz4
Format0=n+
List=%P %aQ
Extract=lz4 {%S} -d %AQ %FQ
Add=lz4 {%S} -f %FQ %AQ
```
Script [cutext](scripts/cutext)

---
<a name="lzop"><h3>lzop</h3></a>
```ini
[lzop]
Archiver=$COMMANDER_PATH/scripts/cutext
Description=lzop
Extension=lzo
Format0=n+
List=%P %AQ
Extract=lzop -d %AQ -o%FQ
Add=lzop {%S} -f %FQ -o%AQ
```
[lzop](http://www.lzop.org/), script [cutext](scripts/cutext)

---
<a name="mcm"><h3>mcm</h3></a>
```ini
[mcm]
Archiver=$COMMANDER_PATH/scripts/cutext
Description=mcm
ID=4D 43 4D 41 52 43 48 49 56 45
IDPos=0
IDSeekRange=0
Extension=mcm
Format0=n+
List=%P %AQ
Extract=mcm d %AQ %FQ
Add=mcm -m9 %FQ %AQ
```
[mcm](https://github.com/mathieuchartier/mcm), script [cutext](scripts/cutext)

---
<a name="cab"><h3>Microsoft Cabinet</h3></a>
```ini
[CAB]
Archiver=cabextract
Description=MSCAB
ID=4D 53 43 46
IDPos=<SeekID>
Start=^-----------
End=All done
Format0=$z+$? dd.tt.yyyy hh:mm:ss ? n+
List=%PQU -l %AQU
Extract=%PQU %AQU
```
```ini
[CAB_2]
Archiver=$COMMANDER_PATH/scripts/unpack_all
Description=MSCAB (script unpack_all)
ID=4D 53 43 46
IDPos=<SeekID>
Start=^-----------
End=All done
Format0=$z+$? dd.tt.yyyy hh:mm:ss ? n+
List=cabextract -l %AQU
Extract=%PQU %LQU 'cabextract %AQU'
```
Script [unpack_all](scripts/unpack_all)
```ini
[CAB_3]
Archiver=$COMMANDER_PATH/scripts/unpack_one
Description=MSCAB (script unpack_one)
ID=4D 53 43 46
IDPos=<SeekID>
Start=^-----------
End=All done
Format0=$z+$? dd.tt.yyyy hh:mm:ss ? n+
List=cabextract -l %AQU
Extract=%PQU %FQU 'cabextract -F %FQU %AQU' 
```
Script [unpack_one](scripts/unpack_one)

---
<a name="msi"><h3>Microsoft Windows Installer</h3></a>
```ini
[MSI]
Archiver=7z
Description=Microsoft Windows Installer
ID=D0 CF 11 E0
IDPos=0
Start=^-------------------
End=^-------------------
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz pppppppppppp  n+
List=%P -r0 l %AQA
Extract=%P x -y {-p%W} {%S} %AQA @%LQU
FormMode=8
```
```ini
[MSI_2]
Archiver=msiextract
Description=Microsoft Windows Installer (msiextract)
ID=D0 CF 11 E0
IDPos=0
Format0=n+
List=%PQU -l %AQU
Extract=%PQU %AQU
```
```ini
[MSI_3]
Archiver=$COMMANDER_PATH/scripts/unpack_all
Description=Microsoft Windows Installer (msiextract, script unpack_all)
ID=D0 CF 11 E0
IDPos=0
Format0=n+
List=msiextract -l %AQU
Extract=%PQU %LQU 'msiextract %AQU'
```
Script [unpack_all](scripts/unpack_all)

---
<a name="szdd"><h3>MS-DOS installation compression</h3></a>
```ini
[SZDD]
Archiver=7z
Description=MS-DOS installation compression
ID=53 5A 44 44 88 F0 27 33, 4B 57 41 4A 88 F0 27 D1
IDPos=0
IDSeekRange=0
Start=^-------------------
End=^-------------------
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz pppppppppppp  n+
List=%P -r0 l %AQA
Extract=%P x -y {-p%W} {%S} %AQA @%LQU
ExtractWithoutPath=%P e -y {-p%W} {%S} %AQA @%LQU
Test=%P t -y {%S} %AQA @%LQU
```

---
<a name="nsis"><h3>Nullsoft Scriptable Install System</h3></a>
```ini
[NSIS]
Archiver=7z
Description=NSIS Unpacker Part 1 (needs 7-Zip >= 4.40)
ID=EF BE AD DE 4E 75 6C 6C 73 6F 66 74 49 6E 73 74
IDPos=4, 516, 1028, 1540, 2052, 2564, 3076, 3588, 4100, 4612, 5124, 5636, 6148, 6660, 7172, 7684, 8196, 8708, 9220, 9732, 10244, 10756, 11268, 11780, 12292, 12804, 13316, 13828, 14340, 14852, 15364, 15876, 16388, 16900, 17412, 17924, 18436, 18948, 19460, 19972, 20484, 20996, 21508, 22020, 22532, 23044, 23556, 24068, 24580, 25092, 25604, 26116, 26628, 27140, 27652, 28164, 28676, 29188, 29700, 30212, 30724, 31236, 31748, 32260, 32772, 33284, 33796, 34308, 34820, 35332, 35844, 36356, 36868, 37380, 37892, 38404, 38916, 39428, 39940, 40452, 40964, 41476, 41988, 42500, 43012, 43524, 44036, 44548, 45060, 45572, 46084, 46596, 47108, 47620, 48132, 48644, 49156, 49668, 50180, 50692, 51204, 51716, 52228, 52740, 53252, 53764, 54276, 54788, 55300, 55812, 56324, 56836, 57348, 57860, 58372, 58884, 59396, 59908, 60420, 60932, 61444, 61956, 62468, 62980, 63492, 64004, 64516, 65028, 65540, 66052, 66564, 67076, 67588, 68100, 68612, 69124, 69636, 70148, 70660, 71172, 71684, 72196, 72708, 73220, 73732, 74244, 74756, 75268, 75780, 76292, 76804, 77316, 77828, 78340, 78852, 79364, 79876, 80388, 80900, 81412, 81924, 82436, 82948, 83460, 83972, 84484, 84996, 85508, 86020, 86532, 87044, 87556, 88068, 88580, 89092, 89604, 90116, 90628, 91140, 91652, 92164, 92676, 93188, 93700, 94212, 94724, 95236, 95748, 96260, 96772, 97284, 97796, 98308, 98820, 99332, 99844, 100356, 100868, 101380, 101892, 102404, 102916, 103428, 103940, 104452, 104964, 105476, 105988, 106500, 107012, 107524, 108036, 108548, 109060, 109572, 110084, 110596, 111108, 111620, 112132, 112644, 113156, 113668, 114180, 114692, 115204, 115716, 116228, 116740, 117252, 117764, 118276, 118788, 119300, 119812, 120324, 120836, 121348, 121860, 122372, 122884, 123396, 123908, 124420, 124932, 125444, 125956, 126468, 126980, 127492, 128004, 128516, 129028, 129540, 130052, 130564, 131076, 131588, 132100, 132612, 133124, 133636, 134148, 134660, 135172, 135684, 136196, 136708, 137220, 137732, 138244, 138756, 139268, 139780, 140292, 140804, 141316, 141828, 142340, 142852, 143364, 143876, 144388, 144900, 145412, 145924, 146436, 146948, 147460, 147972, 148484, 148996, 149508, 150020, 150532, 151044, 151556, 152068, 152580, 153092, 153604, 154116, 154628, 155140, 155652, 156164, 156676, 157188, 157700, 158212, 158724, 159236, 159748, 160260, 160772, 161284, 161796, 162308, 162820, 163332, 163844, 164356, 164868, 165380, 165892, 166404, 166916, 167428, 167940, 168452, 168964, 169476, 169988, 170500, 171012, 171524, 172036, 172548, 173060, 173572, 174084, 174596, 175108, 175620, 176132, 176644, 177156, 177668, 178180, 178692, 179204, 179716, 180228, 180740, 181252, 181764, 182276, 182788, 183300, 183812, 184324, 184836, 185348, 185860, 186372, 186884, 187396, 187908, 188420, 188932, 189444, 189956, 190468, 190980, 191492, 192004, 192516, 193028, 193540, 194052, 194564, 195076, 195588, 196100, 196612, 197124, 197636, 198148, 198660, 199172, 199684, 200196, 200708, 201220, 201732, 202244, 202756, 203268, 203780, 204292, 204804, 205316, 205828, 206340, 206852, 207364, 207876, 208388, 208900, 209412, 209924, 210436, 210948, 211460, 211972, 212484, 212996, 213508, 214020, 214532, 215044, 215556, 216068, 216580, 217092, 217604, 218116, 218628, 219140, 219652, 220164, 220676, 221188, 221700, 222212, 222724, 223236, 223748, 224260, 224772, 225284, 225796, 226308, 226820, 227332, 227844, 228356, 228868, 229380, 229892, 230404, 230916, 231428, 231940, 232452, 232964, 233476, 233988, 234500, 235012, 235524, 236036, 236548, 237060, 237572, 238084, 238596, 239108, 239620, 240132, 240644, 241156, 241668, 242180, 242692, 243204, 243716, 244228, 244740, 245252, 245764, 246276, 246788, 247300, 247812, 248324, 248836, 249348, 249860, 250372, 250884, 251396, 251908, 252420, 252932, 253444, 253956, 254468, 254980, 255492, 256004, 256516, 257028, 257540, 258052, 258564, 259076, 259588, 260100, 260612, 261124, 261636, 262148, 262660, 263172, 263684, 264196, 264708, 265220, 265732, 266244, 266756, 267268, 267780, 268292, 268804, 269316, 269828, 270340
Start=^-------------------
End=^-------------------
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz pppppppppppp  n+
List=%P -r0 l %AQA
Extract=%P x -y {-p%W} {%S} %AQA @%LQU
FormMode=8
```
```ini
[NSISSkipSfxHeader]
Archiver=7z
Description=NSIS SkipSfxHeader (using 7-Zip 4.40 and up)
ID=00 00 00 00 EF BE AD DE 4E 75 6C 6C 73 6F 66 74 49 6E 73 74, 08 00 00 00 EF BE AD DE 4E 75 6C 6C 73 6F 66 74 49 6E 73 74
Start=^-------------------
End=^-------------------
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz pppppppppppp  n+
List=%P -r0 l %AQA
Extract=%P x -y {-p%W} {%S} %AQA @%LQU
FormMode=8
```

---
<a name="pakextract"><h3>pakextract</h3></a>
```ini
[PAK]
Archiver=pakextract
Description=Quake/Quake II pak file or Sin .sin
ID=50 41 43 4B
IDPos=0
IDSeekRange=0
Extension=sin
Format0=n+ (z+
List=%PQU -l %AQU
Extract=%PQU %AQU
```
```ini
[PAK_2]
Archiver=$COMMANDER_PATH/scripts/unpack_all
Description=Quake/Quake II pak file or Sin .sin (script unpack_all)
ID=50 41 43 4B
IDPos=0
IDSeekRange=0
Extension=sin
Format0=n+ (z+
List=pakextract -l %AQ
Extract=%PQU %LQU 'pakextract %AQU'
```
[pakextract](http://github.com/yquake2/pakextract), script [unpack_all](scripts/unpack_all)

---
<a name="paq8o9"><h3>PAQ8</h3></a>
```ini
[paq8o]
Archiver=$COMMANDER_PATH/scripts/cutext
Description=PAQ8
Extension=paq8o9
Format0=n+
List=%P %AQ
Extract=paq8o {%S} -d %AQ %RQ
Add=paq8o {%S} %FQ %AQ
```
Script [cutext](scripts/cutext)

---
<a name="quad"><h3>QUAD</h3></a>
```ini
[QUAD]
Archiver=$COMMANDER_PATH/scripts/cutext
Description=QUAD
Extension=quad
Format0=n+
List=%P %AQ
Extract=quad -d %AQ %FQ
Add=quad {%S} -f %FQ %AQ
```
[QUAD](https://sourceforge.net/projects/quad/), script [cutext](scripts/cutext)

---
<a name="rar"><h3>RAR + 7Zip</h3></a>
```ini
[RAR_3]
Archiver=rar
Description=RAR 5.x - http://www.rarlab.com
ID=52 61 72 21
IDPos=<SeekID>
Extension=rar
Start=^-------------------
End=^-------------------
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz pppppppppppp  n+
List=7z -r0 l {-p%W} %AQA
Extract=7z x -y {-p%W} {%S} %AQA @%LQU
ExtractWithoutPath=7z e -y {-p%W} {%S} %AQA @%LQU
Test=%P t -y {%S} %AQA
Delete=%P d -y {%S} %AQA @%LQA
Add=%P a -y {-p%W} {-v%V} {%S} %AQA @%LQA
AddSelfExtract=%P a -y -sfx {-p%W} {-v%V} {%S} %AQA @%LQA
PasswordQuery=Enter password
FormMode=8
```

---
<a name="unar"><h3>The Unarchiver</h3></a>
```ini
[Unarchiver]
Archiver=unar
Description=The Unarchiver - https://theunarchiver.com/
Extension=sit,sitx,zoo,cpt,sea,ark,sue,pak
Start======
End=^(Flags
Format0=$* aaaaa  $z+$*$*$yyyy-tt-dd?hh?mm$n+
List=lsar -l %AQ
Extract=unar -f -t -D {-p %W} %AQ
PasswordQuery=Password (will not be shown):
FormMode=8
```
```ini
[Unarchiver_2]
Archiver=unar
Description=The Unarchiver - https://theunarchiver.com/
ID=25 50 44 46, D0 CF 11 E0 A1 B1 1A E1, 43 57 53
IDPos=0, <SeekID>
Start======
End=^(Flags
Format0=$* aaaaa  $z+$*$*$*$n+
List=lsar -l %AQ
Extract=unar -f -t -D {-p %W} %AQ %FQ
PasswordQuery=Password (will not be shown):
FormMode=8
```
```ini
[ARC]
Archiver=unar
Description=old ARC via The Unarchiver - https://theunarchiver.com/
ID=1A 02, 1A 03, 1A 04, 1A 08, 1A 09
IDPos=0
IDSeekRange=0
Extension=
Start======
End=^(Flags
Format0=$* aaaaa  $z+$*$*$yyyy-tt-dd?hh?mm$n+
List=lsar -l %AQ
Extract=unar -f -t -D {-p %W} %AQ %FQ
FormMode=8
```
[The Unarchiver](https://theunarchiver.com/)

---
<a name="unace"><h3>UNACE</h3></a>
```ini
[ACE (ro)]
Archiver=unace
Description=UNACE v2.5
ID=2A 2A 41 43 45 2A 2A
IDPos=<SeekID>
Extension=ace
Start=^  Date
End=^listed:
Format0=dd.tt.yy hh:mm ppppppppppp zzzzzzzzz        nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
List=%P v -y %AQA
Extract=%P x -y {-p%W} {%S} %AQA @%LQA
ExtractWithoutPath=%P e -y {-p%W} {%S} %AQA @%LQA
Test=%P t -y %AQA
```

---
<a name="upx"><h3>UPX</h3></a>
```ini
[UPX]
Archiver=upx
Description=UPX 3.94
ID=55 50 58 21
IDPos=<SeekID>
Extension=upx
Start=------
End=
Format0=$z+ ??$p+$??????$*$n+
List=%P -q -l %AQW
Extract=%P -d -o %RQ%FQ %AQ
ExtractWithoutPath=
Test=%P -t -q %AQW
Add=%P {%S} -q -o %AQ %FQ
```

---
<a name="uue"><h3>UUEncode</h3></a>
```ini
[UUEncode]
Archiver=%COMMANDER_PATH%/utils/base64uue
Description=UUEncode
Extension=uue
Format0=yyyy tt dd hh mm ss aaaaa zzzzzzzzzzzz n+
List=%P -l %AQ
Extract=%P -d %AQ %FQ
Add=%P -eu %AQ %FQ
```
Script [base64uue](utils/base64uue), [forum](https://doublecmd.sourceforge.io/forum/viewtopic.php?p=24877#p24877)

---
<a name="zpaq"><h3>ZPAQ</h3></a>
```ini
[ZPAQ]
Archiver=zpaq
Description=ZPAQ - http://mattmahoney.net/dc/zpaq.html
IDSeekRange=0
Extension=zpaq
Start=
End=shown
Format0=- yyyy-tt-dd hh:mm:ss$z+$aaaaa$n+
List=%P l %AQA
Extract=%P x %AQA {%S} {-key %W}
ExtractWithoutPath=
Test=%P l %AQA -test
Add=%P a %AQA %FQU {%S} {-key %W}
FormMode=1
```
[ZPAQ](http://mattmahoney.net/dc/zpaq.html)

---
<a name="zstd"><h3>ZSTD</h3></a>
```ini
[ZSTD]
Archiver=$COMMANDER_PATH/scripts/cutext
Description=Zstandard - http://www.zstd.net
Extension=zst
Format0=n+
List=%P %aQ
Extract=zstd {%S} -d %AQ -o %FQ
Add=zstd {%S} -f %FQ -o %AQ
```
Script [cutext](scripts/cutext)

```ini
[TZST]
Archiver=tar
Description=Compressed tar file (tar.zst)
Extension=tzst
Format0=aaaaaaaaaa zzzzzzz yyyy-tt-dd hh:mm nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
List=%P -tv -I zstd -f %AQA
Extract=%P -x -I zstd {%S} -f %AQA -T %LFQA
Add=%P -c -I zstd {%S} -f %AQA --no-recursion -T %LQA %E512
```
