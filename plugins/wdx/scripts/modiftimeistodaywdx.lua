-- modiftimeistodaywdx.lua (cross-platform)
-- 2019.08.01
--
-- The last modified time is today (not last twenty-four hours), boolean

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Today", "", 6
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  local t = string.sub(FileName, string.len(FileName) - 3, -1)
  if t == SysUtils.PathDelim .. ".." then return nil end
  t = string.sub(t, 2, -1)
  if t == SysUtils.PathDelim .. "." then return nil end
  if FieldIndex == 0 then
    local h, f = SysUtils.FindFirst(FileName)
    if h == nil then return nil end
    SysUtils.FindClose(h)
    local ft = os.date('*t', f.Time)
    local ct = os.date('*t')
    if (ft.year == ct.year) and (ft.month == ct.month) and (ft.day == ct.day) then
      return true
    else
      return false
    end
  end
  return nil
end
