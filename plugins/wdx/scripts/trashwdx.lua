-- trashwdx.lua (cross-platform)
-- 2021.11.19
--[[
Getting info about deleted (moved to trash) files: original filename and deletion date.
Use in a trash directory!

Supported fields: see table "fields".

URI decode: http://lua-users.org/wiki/StringRecipes
]]

local fields = {
{"Original filename",      "", 8},
{"Parts of full filename", "full|path|name|base name|extension", 8},
{"Deletion date",          "", 8},
{"Not older than:",        "today|12 hours|1 day|3 days|1 week", 6}
}
local atps = {
"[/\\]%.local[/\\]share[/\\]Trash[/\\]files[/\\]",
"[/\\]%.Trash%-%d+[/\\]files[/\\]",
"[/\\]%.Trash[/\\]%d+[/\\]files[/\\]"
}
-- 12 hours, 1 day, 3 days, 1 week
local atm ={43200, 86400, 259200, 604800}

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  local fn = SysUtils.ExtractFileName(FileName)
  if (fn == '.') or (fn == '..') then return nil end
  if CheckPath(FileName) == nil then return nil end
  local tmp = SysUtils.ExtractFileDir(FileName)
  if SysUtils.ExtractFileName(tmp) ~= 'files' then return nil end
  local ft = SysUtils.ExtractFileDir(tmp) .. SysUtils.PathDelim .. 'info' .. SysUtils.PathDelim .. fn .. '.trashinfo'
  local h = io.open(ft, 'r')
  if h == nil then return nil end
  local ftc = h:read('*a')
  h:close()
  fn = string.match(ftc, 'Path=([^\r\n]+)')
  if string.find(fn, '%', 1, true) ~= nil then
    fn = string.gsub(fn, "%%(%x%x)",
            function(h) return string.char(tonumber(h,16)) end)
  end
  if FieldIndex == 0 then
    return SysUtils.ExtractFileName(fn)
  elseif FieldIndex == 1 then
    if UnitIndex == 0 then
      return fn
    elseif UnitIndex == 1 then
      return SysUtils.ExtractFileDir(fn)
    elseif UnitIndex == 2 then
      return SysUtils.ExtractFileName(fn)
    elseif UnitIndex == 3 then
      local t = SysUtils.ExtractFileName(fn)
      local e = SysUtils.ExtractFileExt(t)
      if string.len(e) > 1 then
        return string.sub(t, 1,  string.len(t) - string.len(e))
      else
        return t
      end
    elseif UnitIndex == 4 then
      local t = SysUtils.ExtractFileExt(fn)
      if string.len(t) > 1 then
        return string.sub(t, 2, -1)
      else
        return nil
      end
    end
  else
    local ds = string.match(ftc, 'DeletionDate=([^\r\n]+)')
    if FieldIndex == 2 then
      return string.gsub(ds, 'T', ' ')
    elseif FieldIndex == 3 then
      local dd = {}
      dd.year, dd.month, dd.day, dd.hour, dd.min, dd.sec = string.match(ds, '(%d%d%d%d)%-(%d%d)%-(%d%d)T(%d%d):(%d%d):(%d%d)')
      for k, v in pairs(dd) do dd[k] = tonumber(v) end
      if UnitIndex == 0 then --today
        local cd = os.date('*t')
        if (dd.year == cd.year) and (dd.month == cd.month) and (dd.day == cd.day) then
          return true
        else
          return false
        end
      else
        local cd = os.time()
        local i = cd - os.time(dd)
        if atm[UnitIndex + 1] ~= nil then
          if i < atm[UnitIndex + 1] then return true else return false end
        end
      end
    end
  end
  return nil
end

function CheckPath(s)
  for i = 1, #atps do
    if string.find(s, atps[i]) ~= nil then return true end
  end
  return nil
end
