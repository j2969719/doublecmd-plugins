-- caseduplwdx.lua (cross-platform)
-- 2026.04.06
--[[
Checks the existence of files with the same names, but with a different letter case.
(Like `filenamechrstatwdx.lua`, but without the `luautf8` module.)
]]

function ContentGetSupportedField(FieldIndex)
  if (FieldIndex == 0) then
    return "Case duplicates", "", 6; -- ft_boolean
  end
  return "", "", 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if (FieldIndex == 0) then
    local IsFound = false;
    local CurrentName = FileName:match("[" .. SysUtils.PathDelim .. "]([^" .. SysUtils.PathDelim .. "]+)$");
    if (CurrentName == "..") or (CurrentName == ".") then return nil end;
    local CurrentNameLow = LazUtf8.LowerCase(CurrentName);
    local CurrentPath = FileName:match("(.*[" .. SysUtils.PathDelim .. "])");
    local Handle, FindData = SysUtils.FindFirst(CurrentPath .. "*");
    if (Handle ~= nil) then
      repeat
        if (FindData.Name ~= CurrentName) and (LazUtf8.LowerCase(FindData.Name) == CurrentNameLow) then
          IsFound = true;
          break;
        end
        Result, FindData = SysUtils.FindNext(Handle)
      until (Result == nil)
      SysUtils.FindClose(Handle)
      return IsFound;
    end
  end
  return nil; -- invalid
end
