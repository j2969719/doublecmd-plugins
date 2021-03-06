-- markerwdx.lua (cross-platform)
-- 2020.09.20

local dbn
local ft, c = 0, 0
local fl = {}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Color", "", 8
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
  local at = SysUtils.FileGetAttr(FileName)
  if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
  if dbn == nil then
    local sn = debug.getinfo(1).source
    if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
    local pt = ".*\\"
    if SysUtils.PathDelim == "/" then pt = "/.*/" end
    local i, j = string.find(sn, pt)
    if i ~= nil then
      dbn = string.sub(sn, i, j) .. 'marker.txt'
    else
      return nil
    end
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
  if c >= 1 then
    local n, s
    for i = 1, c do
      n = string.find(fl[i], '=', 1, true)
      if n == nil then break end
      s = string.sub(fl[i], 1, n - 1)
      if s == FileName then return string.sub(fl[i], n + 1, -1) end
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
    if fl[c] == '' then c = c - 1 end
  end
end
