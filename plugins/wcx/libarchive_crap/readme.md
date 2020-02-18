## Notes
- abandoned
- potentially dangerous
- some limitations


|ext|flags|notes|
|---|---|---|
|`7z`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`ar`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`b64u`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [ ] modify existing archives</li></ul>|base64-variant of the uuencode format|
|`bz2`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`cpio`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`grz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`gz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`iso`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`lha`|<ul><li>- [ ] create new archives</li><li>- [ ] contain multiple files</li><li>- [ ] modify existing archives</li></ul>||
|`liz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`lrz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`lz4`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`lz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`lzma`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`lzo`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`mtree`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>|can read files from fs (if enabled in read options)|
|`rar`|<ul><li>- [ ] create new archives</li><li>- [ ] contain multiple files</li><li>- [ ] modify existing archives</li></ul>||
|`shar`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [ ] modify existing archives</li></ul>|write only|
|`tar`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`tbz2`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`tgz`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`txz`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`tzst`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`uue`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [ ] modify existing archives</li></ul>||
|`warc`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`xar`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`xz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`z`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||
|`zip`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li></ul>|also support encryption|
|`zst`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li></ul>||

**Note about minimal `libarchive` version:**
- 64-bit AR: `libarchive` >= 3.4.0;
- RAR 5.0: `libarchive` >= 3.4.0;
- ZIPX with xz, lzma, ppmd8 and bzip2 compression: `libarchive` >= 3.4.0;
- ZStandard: `libarchive` >= 3.3.3.
