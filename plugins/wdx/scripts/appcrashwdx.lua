-- appcrashwdx.lua (cross-platform)
-- 2021.04.18
--[[
Getting info from CRASH-files on Debian/Ubuntu and Debian/Ubuntu-based distributions (/var/crash/*.crash).
Only app crash files, not core dumps!
]]

local un = "year|month|day|hour|minute|second"
local fields = {
{"Date",              "Date",           "", 8},
{"Date as digits",    "Date",           un, 1},
{"Problem type",      "ProblemType",    "", 8},
{"Distro",            "DistroRelease",  "", 8},
{"Kernel/machine",    "Uname",          "", 8},
{"Architecture",      "Architecture",   "", 8},
{"Name",              "ExecutablePath", "", 8},
{"Path",              "ExecutablePath", "", 8},
{"Command line",      "ProcCmdline",    "", 8},
{"Working directory", "ProcCwd",        "", 8}
}
local ad = {
{1, 4},
{6, 7},
{9, 10},
{12, 13},
{15, 16},
{18, 19}
}
local filename = ""
local fc = ""

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][3], fields[FieldIndex + 1][4]
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="CRASH"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if e ~= '.crash' then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    local o
    fc = h:read(30720)
    if (fc ~= nil) and (fc ~= '') then
      if string.find(fc, '\nCoreDump:', 1, true) ~= nil then
        o = true
      else
        local tmp
        local b = 30720
        while true do
          tmp = h:read(30720)
          if tmp == nil then break end
          fc = fc .. tmp
          if string.find(fc, '\nCoreDump:', b - 11, true) ~= nil then
            o = true
            break
          end
          b = b + 30720
        end
      end
    end
    h:close()
    if o ~= true then return nil end
    filename = FileName
  end
  local n1, n2
  if FieldIndex == 2 then
    n1, n2 = string.find(fc, '^ProblemType:', 1, false)
  else
    n1, n2 = string.find(fc, '\n' .. fields[FieldIndex + 1][2] .. ':', 1, true)
  end
  if n1 == nil then return nil end
  n1 = string.find(fc, '\n', n2, true)
  local s =  string.sub(fc, n2 + 1, n1 - 1)
  s = string.gsub(s, '^[\t ]+', '')
  if (FieldIndex == 0) or (FieldIndex == 1) then
    local t = FormatDate(s)
    if FieldIndex == 0 then
      return t
    else
      if UnitIndex < 6 then
        return tonumber(string.sub(t, ad[UnitIndex + 1][1], ad[UnitIndex + 1][2]))
      end
    end
  elseif FieldIndex == 6 then
    local i, j = string.find(s, '/.*/')
    if i == nil then return nil end
    return string.sub(s, j + 1, -1)
  else
    return s
  end
  return nil
end

function FormatDate(s)
  local m = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}
  local dt = {}
  dt.month, dt.day, dt.hour, dt.min, dt.sec, dt.year = string.match(s, '([A-Za-z]+)%s+(%d+)%s+(%d+):(%d+):(%d+)%s+(%d%d%d%d)')
  for i = 1, #m do
    if dt.month == m[i] then
      dt.month = i
      break
    end
  end
  for k, v in pairs(dt) do dt[k] = tonumber(v) end
  dti = os.time(dt)
  return os.date('%Y.%m.%d %H:%M:%S', dti)
end
