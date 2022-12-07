-- tagswdx.lua (cross-platform)
-- 2022.12.06
--[[
Maybe try to use "os.setenv" (but DC 1.1.0+) instead of "SysUtils.Find*" ??? This is the narrowest place.
]]

local dbn
local ft, c = 0, 0
local fl = {}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Tags", "", 8
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
  if flags == 1 then return nil end; -- Исключаем вывод для диалога свойств (CONTENT_DELAYIFSLOW)
  if FieldIndex ~= 0 then return nil end
  if dbn == nil then
    local sn = debug.getinfo(1).source
    if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
    dbn = SysUtils.ExtractFilePath(sn) .. 'tags.txt'
  end
  if #fl == 0 then
    ReadFileList()
  else
    local h, fd = SysUtils.FindFirst(dbn)
    if h == nil then return nil end
    SysUtils.FindClose(h)
    if fd.Time > ft then
      ReadFileList()
      ft = fd.Time
    end
  end
  if c > 0 then
    local n, s
    for i = 1, c do
      n = string.find(fl[i], '|', 1, true)
      if n == nil then break end
      s = string.sub(fl[i], 1, n - 1)
      if s == FileName then
        s = string.sub(fl[i], n + 1, -2)
        s = string.gsub(s, '|', ', ')
        return s
      end
    end
  end
  return nil
end

function ReadFileList()
  local h = io.open(dbn, 'r')
  if h == nil then
    c = 0
  else
    c = 1
    for l in h:lines() do
      fl[c] = l
      c = c + 1
    end
    h:close()
    c = c - 1
  end
end
