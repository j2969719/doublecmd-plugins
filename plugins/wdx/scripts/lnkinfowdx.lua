-- lnkinfowdx.lua
-- 2021.04.18
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
    local e = SysUtils.ExtractFileExt(FileName)
    if e ~= '.lnk' then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    h = io.popen('lnkinfo "' .. FileName:gsub('"', '\\"') .. '"')
    if h == nil then return nil end
    out = h:read('*a')
    h:close()
    if out == nil then return nil end
    filename = FileName
  end
  if fields[FieldIndex + 1] ~= nil then
    return string.match(out, '[\t ]' .. fields[FieldIndex + 1] .. '[\t ]+:[\t ]+([^\r\n]+)')
  end
  return nil
end
