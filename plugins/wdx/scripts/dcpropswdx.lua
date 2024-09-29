local ft_number = 2
local ft_string = 8
local ft_date = 10

local fields = {-- field name, field type
  {"Size",           ft_number},
  {"Attributes",     ft_string},
  {"Group",          ft_string},
  {"Owner",          ft_string},
  {"ModificationTime", ft_date},
  {"CreationTime",     ft_date},
  {"LastAccessTime",   ft_date},
  {"ChangeTime",       ft_date},
  {"ChangeTime",       ft_date},  
  {"Type",           ft_string},
  {"Comment",        ft_string},
--{"LinkTo",         ft_string},  
}

function ContentGetSupportedField(FieldIndex)
  if (fields[FieldIndex + 1] ~= nil) then
    return fields[FieldIndex + 1][1], '', fields[FieldIndex + 1][2]
  end
  return '', '', 0 -- ft_nomorefields
end

function ContentGetDetectString()
  return 'EXT="*"' -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  local result = SysUtils.GetFileProperty(FileName, FieldIndex)
  if not result then
    return nil
  end
  local fieldtype = fields[FieldIndex + 1][2]
  if (fieldtype == ft_date) then
    return result * 10000000 + 116444736000000000
  else
    return result
  end
  return nil -- invalid
end
