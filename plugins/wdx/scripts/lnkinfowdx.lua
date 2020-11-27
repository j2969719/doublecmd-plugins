-- lnkinfowdx.lua
-- 2020.10.30
--[[
Getting some information from Windows shell link (shortcut) file.

For Unix-like only!

Requires "lnkinfo": package "liblnk-utils" (Debian/Ubuntu) or "liblnk-tools" (Arch Linux) and so on
]]

local fields = {
"Local path",
"Description",
"Relative path",
"Command line arguments",
"Working directory"
}
local filename = ''
local out

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1], "", 8
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="LNK"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000004) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000400) % 2 ~= 0 then return nil end
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    local d = h:read(20)
    h:close()
    if (BinToHex(d, 1, 4, '') == 0x4c000000) and (BinToHex(d, 5, 8, '') == 0x01140200) and (BinToHex(d, 9, 12, '') == 0x00000000) and (BinToHex(d, 13, 16, '') == 0xc0000000) and (BinToHex(d, 17, 20, '') == 0x00000046) then
      h = io.popen('lnkinfo "' .. FileName:gsub('"', '\\"') .. '"')
      out = h:read('*a')
      h:close()
    else
      return nil
    end
    filename = FileName
  end
  if (out ~= nil) and (fields[FieldIndex + 1] ~= nil) then
    return string.match(out, '[\t ]' .. fields[FieldIndex + 1] .. '[\t ]+:[\t ]+([^\r\n]+)')
  end
  return nil
end

function BinToHex(d, n1, n2, e)
  local r = ''
  if e == 'le' then
    for j = n1, n2 do r = string.format('%02x', string.byte(d, j)) .. r end
  else
    for j = n1, n2 do r = r .. string.format('%02x', string.byte(d, j)) end
  end
  return tonumber('0x' .. r)
end
