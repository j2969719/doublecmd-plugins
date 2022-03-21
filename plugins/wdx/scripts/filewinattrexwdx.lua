-- filewinattrexwdx.lua (Windows only)
-- 2022.03.21
--[[
Return file attributes (Windows only!)

  A - Archive;
  C - Compressed;
  D - Directory;
  E - Encrypted;
  H - Hidden;
  I - Not content indexed;
  L - Symbolic link/Junction/Mount point;
  O - Offline;
  P - Pinned (OneDrive "always available files");
  R - Read-only;
  S - System;
  T - Temporary;
  U - Unpinned (OneDrive "online-only files");
  V - Integrity;
  X - No scrub.

as string "ACDEHILOPRSTUVX" (i.e. only set attributes).
]]

local ffi = require("ffi")
local flib = ffi.load("kernel32")

ffi.cdef[[
int GetFileAttributesW(const char *lpFileName);
]]

local afattr = {
{0x00000020, "A"},
{0x00000800, "C"},
{0x00000010, "D"},
{0x00004000, "E"},
{0x00000002, "H"},
{0x00002000, "I"},
{0x00000400, "L"},
{0x00001000, "O"},
{0x00080000, "P"},
{0x00000001, "R"},
{0x00000004, "S"},
{0x00000100, "T"},
{0x00100000, "U"},
{0x00008000, "V"},
{0x00020000, "X"}
}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "AttrEx", "", 8
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex == 0 then
    local tmp = LazUtf8.ConvertEncoding('\\\\?\\' .. FileName, 'utf8', 'ucs2le')
    local attr = flib.GetFileAttributesW(tmp)
    if attr < 0 then return nil end
    -- Skip directories
    --if math.floor(attr / 0x00000010) % 2 ~= 0 then return nil end
    tmp = ''
    for i = 1, #afattr do
      if math.floor(attr / afattr[i][1]) % 2 ~= 0 then tmp = tmp .. afattr[i][2] end
    end
    if tmp ~= '' then return tmp end
  end
  return nil
end
