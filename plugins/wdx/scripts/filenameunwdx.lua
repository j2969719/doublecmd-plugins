-- filenameunwdx.lua (cross-platform)
-- 2023.07.23
--[[
Returns the normalized (Unicode normalization) filename:
- form C (NFC);
- form D (NFD);
- form KC (NFKC);
- form KC (NFKD)
(Base name is a filename without extension.)

The "Contains combined chars" field has been added to search for files
whose names contain combined characters.

Requires LuaJIT and the libunistring library (https://www.gnu.org/software/libunistring/).
- This library is usually installed in Linux.
- Windows: use the DLL file from Git for Windows, MinGW, GIMP and so on.
          Or just try NFCname (http://totalcmd.net/plugring/NFCname.html).
- The list of library file names is listed in the "alib" table.

UTF8Normalization(): part of https://github.com/bungle/lua-resty-unistring
]]

local ffi = require("ffi")

local alib = {
"libunistring.so",
"libunistring.so.2",
"libunistring.so.5",
"libunistring.dll",
"libunistring-2.dll",
"libunistring-5.dll"
}
local br, lib = pcall(ffi.load, "unistring")
if br == false then
  for i = 1, #alib do
    br, lib = pcall(ffi.load, alib[i])
    if br == true then break end
  end
end

ffi.cdef[[
struct unicode_normalization_form;
typedef const struct unicode_normalization_form *uninorm_t;
const struct unicode_normalization_form uninorm_nfc;
const struct unicode_normalization_form uninorm_nfd;
const struct unicode_normalization_form uninorm_nfkc;
const struct unicode_normalization_form uninorm_nfkd;
uint8_t * u8_normalize(uninorm_t nf, const uint8_t *s, size_t n, uint8_t *resultbuf, size_t *lengthp);
]]
local form = {
 lib.uninorm_nfc,
 lib.uninorm_nfd,
 lib.uninorm_nfkc,
 lib.uninorm_nfkd
}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Name", "NFC|NFD|NFKC|NFKD", 8
  elseif FieldIndex == 1 then
    return "Base name", "NFC|NFD|NFKC|NFKD", 8
  elseif FieldIndex == 2 then
    return "Contains combined chars", "", 6
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if br == false then return nil end
  local fn = SysUtils.ExtractFileName(FileName)
  if FieldIndex == 0 then
    if UnitIndex > 3 then
      return nil
    else
      return UTF8Normalization(UnitIndex + 1, fn)
    end
  elseif FieldIndex == 1 then
    if UnitIndex > 3 then
      return nil
    else
      local fnb = string.gsub(fn, "^(.+)%.[^%.]+$", "%1")
      return UTF8Normalization(UnitIndex + 1, fnb)
    end
  elseif FieldIndex == 2 then
    local st = UTF8Normalization(0, fn)
    if fn == st then
      return false
    else
      return true
    end
  else
    return nil
  end
end

function UTF8Normalization(f, s)
  local size = ffi.new "size_t[1]"
  local uint8t = ffi.typeof "uint8_t[?]"
  local nf = form[f]
  local l = string.len(s)
  lib.u8_normalize(nf, s, l, nil, size)
  local b = ffi.new(uint8t, size[0])
  lib.u8_normalize(nf, s, l, b, size)
  return ffi.string(b, size[0])
end
