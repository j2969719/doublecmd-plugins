-- dcpropswdx.lua (cross-platform)
-- 2026.06.14
--[[
Returns some file properties, see SysUtils.GetFileProperty
https://doublecmd.github.io/doc/en/lua.html#sysutils_getfileproperty
]]

-- field name, field type
local fields = {
{"Size",              2},
{"Attributes",        8},
{"Group",             8},
{"Owner",             8},
{"ModificationTime", 10},
{"CreationTime",     10},
{"LastAccessTime",   10},
{"ChangeTime",       10},
{"Type",              8},
{"Comment",           8},
}

function ContentGetSupportedField(FieldIndex)
  if (fields[FieldIndex + 1] ~= nil) then
    return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2]
  end
  return "", "", 0 -- ft_nomorefields
end

function ContentGetDetectString()
  return 'EXT="*"' -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  local result = SysUtils.GetFileProperty(FileName, FieldIndex)
  if not result then return nil end
  if fields[FieldIndex + 1][2] == 10 then
    return result * 10000000 + 116444736000000000
  else
    return result
  end
end
