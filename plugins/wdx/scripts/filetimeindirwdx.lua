-- filetimeindirwdx.lua (cross-platform)
-- 2020.04.05
--[[
Returns date and time of oldest and newest files in directory.
Without scanning symbolic links to folders, symlinks to file will be ignored.

Only for columns set or tooltips!

"Newest file time" and "Oldest file time" returns date in your regional format.
"As string" returns date as string YYYY.MM.DD HH:MM:SS.

Note: "Newest file time" and "Oldest file time" returns date as ft_datetime: you
need DC >= r9367 and DC x64 or DC x32 but with Lua 5.3. Otherwise you can just
use the "As string" field.
]]

local fields = {
 {"Newest file time", "", 10},
 {"Oldest file time", "", 10},
 {"As string", "newest|oldest", 8}
}
local tmax, tmin

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]
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
  if FieldIndex > 2 then return nil end
  if flags == 1 then return nil end; -- Исключаем вывод для диалога свойств (CONTENT_DELAYIFSLOW)
  local t = string.sub(FileName, string.len(FileName) - 3, -1)
  if t == SysUtils.PathDelim .. ".." then return nil end
  t = string.sub(t, 2, -1)
  if t == SysUtils.PathDelim .. "." then return nil end
  local k = SysUtils.FileGetAttr(FileName)
  if k > 0 then
    if (math.floor(k / 0x00000400) % 2 ~= 0) then return nil end
    if (math.floor(k / 0x00000010) % 2 ~= 0) then
      tmax = 0
      tmin = os.time() + 86400
      k = tmin
      ScanDirTime(FileName)
      if FieldIndex == 0 then
        if tmax ~= 0 then
          return tmax * 10000000 + 116444736000000000
        end
      elseif FieldIndex == 1 then
        if tmin ~= k then
          return tmin * 10000000 + 116444736000000000
        end
      elseif FieldIndex == 2 then
        if UnitIndex == 0 then
          if tmax ~= 0 then return os.date('%Y.%m.%d %H:%M:%S', tmax) end
        elseif UnitIndex == 1 then
          if tmin ~= k then return os.date('%Y.%m.%d %H:%M:%S', tmin) end
        end
      end
    end
  end
  return nil
end

function ScanDirTime(p)
  local r = nil
  local h, d = SysUtils.FindFirst(p .. SysUtils.PathDelim .. "*")
  if h ~= nil then
    repeat
      if (d.Name ~= ".") and (d.Name ~= "..") then
        if (math.floor(d.Attr / 0x00000400) % 2 == 0) then
          if (math.floor(d.Attr / 0x00000010) % 2 == 0) then
            if tmax < d.Time then tmax = d.Time end
            if tmin > d.Time then tmin = d.Time end
          else
            ScanDirTime(p .. SysUtils.PathDelim .. d.Name)
          end
        end
      end
      r, d = SysUtils.FindNext(h)
    until r == nil
    SysUtils.FindClose(h)
  end
end
