-- agewdx.lua (cross-platform)
-- 2020.10.24
--[[
Returns age (the date of the last modification will be used):
number of seconds, minutes, hours, days, weeks, months or years
Notes:
- it will be the largest integer smaller than or equal to result;
- returns nothing if the current time is less than the file time (i.e. result should be >= 0).
]]

local atd = {0, 0, 0}
local filename = ''

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return 'Age (modif. date)', 'seconds|minutes|hours|days|weeks|months|years', 2
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
  if FieldIndex > 0 then return nil end
  if filename ~= FileName then
    local h, fd = SysUtils.FindFirst(FileName)
    if h == nil then return nil else SysUtils.FindClose(h) end
    if fd.Attr < 0 then return nil end
    if math.floor(fd.Attr / 0x00000004) % 2 ~= 0 then return nil end
    --if math.floor(fd.Attr / 0x00000010) % 2 ~= 0 then return nil end
    --if math.floor(fd.Attr / 0x00000400) % 2 ~= 0 then return nil end
    atd[1] = os.time()
    atd[2] = fd.Time
    atd[3] = atd[1] - atd[2]
    filename = FileName
  end
  if atd[3] == 0 then
    return 0
  elseif atd[3] > 0 then
    if UnitIndex == 0 then
      return atd[3]
    elseif UnitIndex == 1 then
      return math.floor(atd[3] / 60)
    elseif UnitIndex == 2 then
      return math.floor(atd[3] / 3600)
    elseif UnitIndex == 3 then
      return math.floor(atd[3] / 86400)
    elseif UnitIndex == 4 then
      return math.floor(atd[3] / 604800)
    elseif (UnitIndex == 5) or (UnitIndex == 6) then
      local ct = os.date('*t', atd[1])
      local ft = os.date('*t', atd[2])
      local cy = ct.year - ft.year
      if UnitIndex == 5 then
        if cy == 0 then
          return ct.month - ft.month
        else
          return cy * 12 + ct.month - ft.month
        end
      else
        return cy
      end
    end
  end
  return nil
end
