## Notes
|ext|flags|notes|
|---|---|---|
|`7z`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`ar`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`b64u`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [ ] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>|base64-variant of the uuencode format|
|`bz2`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`cpio`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`grz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`gz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`iso`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`lha`|<ul><li>- [ ] create new archives</li><li>- [x] contain multiple files</li><li>- [ ] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`liz`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`lrz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`lz4`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`lz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`lzma`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`lzo`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`mtree`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>|can read files from fs (if enabled in read options)|
|`rar`|<ul><li>- [ ] create new archives</li><li>- [x] contain multiple files</li><li>- [ ] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`shar`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [ ] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>|write only|
|`tar`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`tbz2`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`tgz`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`txz`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`tzst`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`uue`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [ ] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`warc`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`xar`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>||
|`xz`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`z`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||
|`zip`|<ul><li>- [x] create new archives</li><li>- [x] contain multiple files</li><li>- [x] modify existing archives</li><li>- [ ] supports packing in memory</li></ul>|also support encryption|
|`zst`|<ul><li>- [x] create new archives</li><li>- [ ] contain multiple files</li><li>- [x] modify existing archives</li><li>- [x] supports packing in memory</li></ul>||

**Note about minimal `libarchive` version:**
- 64-bit AR: `libarchive` >= 3.4.0;
- RAR 5.0: `libarchive` >= 3.4.0;
- ZIPX with xz, lzma, ppmd8 and bzip2 compression: `libarchive` >= 3.4.0;
- ZStandard: `libarchive` >= 3.3.3.
