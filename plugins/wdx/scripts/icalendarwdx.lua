-- icalendarwdx.lua (cross-platform)
-- 2021.04.18
--[[
iCalendar 2.0
vCalendar 1.0
Some details: https://en.m.wikipedia.org/wiki/ICalendar

Script works if:
- first line is "BEGIN:VCALENDAR";
- some properties are exist:
  VERSION (vCalendar 1.0) or VERSION and PRODID (iCalendar 2.0).

Supported fields: see table "fields".

NOTES:
1. "Component" can be one of the following supported: Event, To-do, Journal entry and Free/busy time.
2. "Starting time" is DTSTART property, "Ending time" is DTEND or DTSTART +/- DURATION (if DTEND was not found and DURATION is exist).
"Oldest" and "newest" will return the more oldest date/time and the more newest date/time among all components.
]]

local fields = {
{"Version",            "", 8},
{"Product identifier", "", 8},
{"Component count", "", 1},
{"Component types", "", 8},
{"Starting time",   "oldest|newest", 8},
{"Ending time",     "oldest|newest", 8}
}

local ar = {}
local filename = ''

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
  return '(EXT="ICS")|(EXT="IFB")|(EXT="VCS")'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if (e ~= '.ics') and (e ~= '.ifb') and (e ~= '.vcs') then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    for j = 1, #ar do ar[j] = nil end
    local s = h:read(15)
    if (s == nil) or (s ~= 'BEGIN:VCALENDAR') then
      h:close()
      return nil
    end
    local all = h:read('*a')
    h:close()
    if all == nil then return nil end
    s = string.match(all, '[\r\n]VERSION:([^\r\n]+)[\r\n]')
    if s == nil then
      return 'Invalid file!'
    else
      ar[1] = s
    end
    s = string.match(all, '[\r\n]PRODID:([^\r\n]+)[\r\n]')
    if s == nil then
      if ar[1] == '1.0' then
        ar[2] = ''
      else
        return 'Invalid file!'
      end
    else
      ar[2] = s
    end
    GetData(all)
    filename = FileName
  end
  if FieldIndex == 0 then
    return ar[1]
  elseif FieldIndex == 1 then
    if ar[2] == '' then return nil else return ar[2] end
  elseif FieldIndex == 2 then
    return ar[3]
  elseif FieldIndex == 3 then
    return ar[4]
  elseif (FieldIndex == 4) or (FieldIndex == 5) then
    if ar[FieldIndex + 1][UnitIndex + 1] == nil then
      return nil
    elseif ar[FieldIndex + 1][UnitIndex + 1] == 0 then
      return nil
    else
      return os.date('%Y.%m.%d %H:%M:%S', ar[FieldIndex + 1][UnitIndex + 1])
    end
  end
  return nil
end

function GetData(all)
  ar[3] = 0
  ar[4] = ''
  ar[5] = {0, 0}
  ar[6] = {0, 0}
  local ae = {}
  local at = {}
  local aj = {}
  local af = {}
  local s, t
  local n1, n2, n3, c, td = 1, 1, 1, 1, 0
  -- VEVENT
  while true do
    n1 = string.find(all, 'BEGIN:VEVENT', n3, true)
    if n1 == nil then
      break
    else
      n2 = string.find(all, 'END:VEVENT', n1, true)
      ae[c] = ClearData(string.sub(all, n1 + 12, n2 + 10))
      c = c + 1
    end
    n3 = n2
  end
  c, n3 = 1, 1
  -- VTODO
  while true do
    n1 = string.find(all, 'BEGIN:VTODO', n3, true)
    if n1 == nil then
      break
    else
      n2 = string.find(all, 'END:VTODO', n1, true)
      at[c] = ClearData(string.sub(all, n1 + 11, n2 + 9))
      c = c + 1
    end
    n3 = n2
  end
  c, n3 = 1, 1
  -- VJOURNAL
  while true do
    n1 = string.find(all, 'BEGIN:VJOURNAL', n3, true)
    if n1 == nil then
      break
    else
      n2 = string.find(all, 'END:VJOURNAL', n1, true)
      aj[c] = ClearData(string.sub(all, n1 + 14, n2 + 12))
      c = c + 1
    end
    n3 = n2
  end
  c, n3 = 1, 1
  -- VFREEBUSY
  while true do
    n1 = string.find(all, 'BEGIN:VFREEBUSY', n3, true)
    if n1 == nil then
      break
    else
      n2 = string.find(all, 'END:VFREEBUSY', n1, true)
      af[c] = ClearData(string.sub(all, n1 + 15, n2 + 13))
      c = c + 1
    end
    n3 = n2
  end
  ar[3] = #ae + #at + #aj + #af
  ar[4] = 'Event (' .. #ae .. '); To-do (' .. #at .. '); Journal entry (' .. #aj .. '); Free/busy time (' .. #af .. ')'
  -- Time
  if #ae == 0 then c = 1 else c = #ae + 1 end
  if #at > 0 then
    for t = 1, #at do
      ae[c] = at[t]
      c = c + 1
    end
  end
  if #aj > 0 then
    for t = 1, #aj do
      ae[c] = aj[t]
      c = c + 1
    end
  end
  if #at > 0 then
    for t = 1, #af do
      ae[c] = af[t]
      c = c + 1
    end
  end
  for c = 1, #ae do
    s = string.match(ae[c], '[\r\n]DTSTART[:;]([^\r\n]+)[\r\n]')
    if s ~= nil then
      t = GetUnixTime(s)
      if t ~= nil then GetOldNewTime(t, 5) end
    end
    s = string.match(ae[c], '[\r\n]DTEND[:;]([^\r\n]+)[\r\n]')
    if s ~= nil then
      t = GetUnixTime(s)
      if t ~= nil then GetOldNewTime(t, 6) end
    else
      if t ~= nil then
        s = string.match(ae[c], '[\r\n]DURATION[:;]([^\r\n]+)[\r\n]')
        if s ~= nil then
          td = GetDuration(s)
          if td ~= 0 then
            if string.find(s, '-P', 1, true) then
              t = t - td
            else
              t = t + td
            end
            td = 0
            GetOldNewTime(t, 6)
          end
        end
      end
    end
  end
end

function ClearData(s)
  local t
  while true do
    t = string.match(s, '[\r\n]BEGIN:([A-Z]%-?[A-Z0-9]+)[\r\n]')
    if t == nil then
      break
    else
      s = string.gsub(s, '[\r\n]+BEGIN:' .. t .. '.+END:' .. t .. '[\r\n]+', '')
    end
  end
  return s
end

function GetDuration(s)
  local td = 0
  local ts = string.match(s, '(%d+)W')
  if (ts ~= nil) and (ts ~= '') then
    td = td + (tonumber(ts) * 604800)
  end
  ts = string.match(s, '(%d+)D')
  if (ts ~= nil) and (ts ~= '') then
    td = td + (tonumber(ts) * 86400)
  end
  ts = string.match(s, '(%d+)H')
  if (ts ~= nil) and (ts ~= '') then
    td = td + (tonumber(ts) * 3600)
  end
  ts = string.match(s, '(%d+)M')
  if (ts ~= nil) and (ts ~= '') then
    td = td + (tonumber(ts) * 60)
  end
  ts = string.match(s, '(%d+)S')
  if (ts ~= nil) and (ts ~= '') then
    td = td + tonumber(ts)
  end
  return td
end

function GetUnixTime(s)
  local dt = {}
  dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = string.match(s, '(%d%d%d%d)(%d%d)(%d%d)T(%d%d)(%d%d)(%d%d)')
  if dt.year == nil then
    dt.year, dt.month, dt.day = string.match(s, '(%d%d%d%d)(%d%d)(%d%d)')
    if dt.year == nil then
      return nil
    else
      dt.hour, dt.min, dt.sec = 0, 0, 0
    end
  else
    if tonumber(dt.year) == 0 then return nil end
  end
  return os.time(dt)
end

function GetOldNewTime(t, i)
  if ar[i][1] == 0 then
    ar[i][1] = t
  else
    if ar[i][1] > t then ar[i][1] = t end
  end
  if ar[i][2] == 0 then
    ar[i][2] = t
  else
    if ar[i][2] < t then ar[i][2] = t end
  end
end
