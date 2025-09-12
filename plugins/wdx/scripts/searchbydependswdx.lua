-- searchbydependswdx.lua (Linux only)
--2025.09.12
--[[
Search by dependencies in ELF (from the .dynamic section).
Requires: ldd.
Only for searching with plugins.
]]

function ContentGetSupportedField(FieldIndex)
  if (FieldIndex == 0) then
    return "Dependencies", "", 9
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if (FieldIndex == 0) and (UnitIndex == 0) then
    local a = SysUtils.FileGetAttr(FileName)
    if a < 0 then return nil end
    if math.floor(a / 0x00000010) % 2 ~= 0 then return nil end
    local h = io.open(FileName, "rb")
    if h == nil then return nil end
    local s = h:read(4)
    h:close()
    if s == nil then return nil end
    if (string.byte(s, 1) == 0x7f) and (string.sub(s, 2, 4) == "ELF") then
      h = io.popen ('ldd "' .. FileName .. '"')
      if h == nil then return nil end
      local d = h:read("*a")
      h:close()
      return d;
    end
  end
  return nil;
end
