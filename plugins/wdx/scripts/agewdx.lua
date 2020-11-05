-- agewdx.lua (cross-platform)
-- 2020.11.06
--[[
Returns age (the date of the last modification will be used).

- "Age"
    number of seconds, minutes, hours, days, weeks, months or years.
- "Float"
    automatic choice of seconds, minutes, hours, days, weeks, months or years
    (i.e. depends on the size of the value).
- "Today"
    returns true if the last modified time is today (not last twenty-four hours).

Notes:
1. All values will be round downward to its nearest integer.
2. Returns nothing if the current time is less than the file time.
]]

local atd = {0, 0, 0}
local filename = ''

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Age", "seconds|minutes|hours|days|weeks|months|years", 2
  elseif FieldIndex == 1 then
    return "Float", "", 8
  elseif FieldIndex == 2 then
    return "Today", "", 6
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
    if atd[3] < 0 then return nil end
    filename = FileName
  end
  if FieldIndex == 0 then
    if atd[3] == 0 then return 0 end
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
  elseif FieldIndex == 1 then
    if atd[3] < 60 then
      return tostring(atd[3]) .. ' sec'; -- < 1 min
    elseif atd[3] < 3600 then
      return tostring(math.floor(atd[3] / 60)) .. ' min'; -- < 1 hour
    elseif atd[3] < 86400 then
      return tostring(math.floor(atd[3] / 3600)) .. ' hour(s)'; -- < 1 day
    elseif atd[3] < 604800 then
      return tostring(math.floor(atd[3] / 86400)) .. ' day(s)'; -- < 1 week
    elseif atd[3] < 2629800 then
      return tostring(math.floor(atd[3] / 604800)) .. ' week(s)'; -- < 1 month
    elseif atd[3] < 31557600 then
      return tostring(math.floor(atd[3] / 2629800)) .. ' month(s)'; -- < 1 year
    else
      return tostring(math.floor(atd[3] / 31557600)) .. ' year(s)'
    end
  elseif FieldIndex == 2 then
    local ct = os.date('*t', atd[1])
    local ft = os.date('*t', atd[2])
    if (ft.year == ct.year) and (ft.month == ct.month) and (ft.day == ct.day) then
      return true
    else
      return false
    end
  end
  return nil
end
