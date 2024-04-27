-- copypasted from https://doublecmd.h1n.ru/viewtopic.php?p=43616#p43616


function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
  --Initialization code here
  local script_path = debug.getinfo(1).source:sub(2)
  local script_dir = SysUtils.ExtractFileDir(script_path)
  local plugname = SysUtils.ExtractFileName(script_path)
  local scripts = {}
  local dir, data = SysUtils.FindFirst(script_dir .. SysUtils.PathDelim .. "*.lua")
  if dir ~= nil then
    repeat
      if data.Name ~= plugname then
        table.insert(scripts, data.Name)
      end
      result, data = SysUtils.FindNext(dir)
    until result == nil
    SysUtils.FindClose(dir)
  end
  table.sort(scripts)
  for i = 1, #scripts do
    print("exec scrpit " .. scripts[i])
    require(scripts[i]:sub(1, -5))
  end
end

function ContentGetSupportedField(FieldIndex)
  if (FieldIndex == 0) then
    return "Autorun", '', 6 -- FieldName,Units,ft_boolean
  end
  return '', '', 0 -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if (FieldIndex == 0) then
    return true -- return fake value
  end
  return nil -- invalid
end

