-- filenamewdx.lua (cross-platform)
-- 2021.10.07
--[[
Returns parts of full filename.
Supported fields: see table "fields".
]]

local fields = {
"Full name",
"Name",
"Base name",
"Extension",
"Path",
"Parent folder"
}

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
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex == 0 then
    return FileName
  elseif FieldIndex == 1 then
    return SysUtils.ExtractFileName(FileName)
  elseif FieldIndex == 2 then
    local t = SysUtils.ExtractFileName(FileName)
    local e = SysUtils.ExtractFileExt(t)
    if string.len(e) > 1 then
      return string.sub(t, 1,  string.len(t) - string.len(e))
    else
      return t
    end
  elseif FieldIndex == 3 then
    local t = SysUtils.ExtractFileExt(FileName)
    if string.len(t) > 1 then
      return string.sub(t, 2, -1)
    else
      return nil
    end
  elseif FieldIndex == 4 then
    return SysUtils.ExtractFileDir(FileName)
  elseif FieldIndex == 5 then
    local t1 = SysUtils.ExtractFileDir(FileName)
    local t2 = SysUtils.ExtractFileDir(t1)
    local t = string.sub(FileName, string.len(t2) + 2, string.len(t1))
    if t == "" then return nil else return t end
  else
    return nil
  end
end
