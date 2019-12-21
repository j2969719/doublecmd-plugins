-- markerwdx.lua (cross-platform)
-- 2019.11.07

local dbn

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
  local h, err = io.open(dbn, 'r')
  if h == nil then return nil end
  local l, n, k, r
  for l in h:lines() do
    n = string.find(l, '=', 1, true)
    if n ~= nil then
      k = string.sub(l, 1, n - 1)
      if k == FileName then
        r = string.sub(l, n + 1)
        break
      end
    end
  end
  h:close()
  return r
end
