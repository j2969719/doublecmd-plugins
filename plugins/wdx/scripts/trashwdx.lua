-- trashwdx.lua (Linux only)
-- 2019.03.21
--
-- Script for using in trash folder:
-- ~/.local/share/Trash/files
-- <mount point>/.Trash/<UID>/files
-- <mount point>/.Trash-<UID>/files
--   where <UID> is user ID

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Original filename", "full|path|name", 8
  elseif FieldIndex == 1 then
    return "Deletion date", "", 8
  elseif FieldIndex == 2 then
    return "Not older than:", "today|12 hours|1 day|3 days|1 week", 6
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if CheckPath(FileName) == nil then return nil end
  local i, j = string.find(FileName, '/.*/')
  if i == nil then return nil end
  local p = string.sub(FileName, i, j - 1)
  local pl = string.len(p)
  if string.sub(p, pl - 5, -1) ~= '/files' then return nil end
  local n = string.sub(FileName, pl + 2, -1)
  if n == '..' then return nil end
  local ti = string.sub(p, 1, -6) .. 'info/' .. n .. '.trashinfo'
  local f = io.open(ti, 'r')
  if f == nil then return nil end
  local fc = f:read('*a')
  f:close()
  if FieldIndex == 0 then
    local fn = string.match(fc, 'Path=([^\r\n]+)')
    if string.find(fn, '%', 1, true) ~= nil then
      -- http://lua-users.org/wiki/StringRecipes
      fn = string.gsub(fn, "%%(%x%x)",
              function(h) return string.char(tonumber(h,16)) end)
    end
    if UnitIndex == 0 then
      return fn
    else
      i, j = string.find(fn, '/.*/')
      if i == nil then return nil end
      p = string.sub(fn, i, j - 1)
      if UnitIndex == 1 then
        return p
      elseif UnitIndex == 2 then
        return string.sub(fn, string.len(p) + 2, -1)
      end
    end
  else
    local ds = string.match(fc, 'DeletionDate=([^\r\n]+)')
    if FieldIndex == 1 then
      return string.gsub(ds, 'T', ' ')
    elseif FieldIndex == 2 then
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
        i = cd - os.time(dd)
        if UnitIndex == 1 then
          j = 43200; -- 12 hours
        elseif UnitIndex == 2 then
          j = 86400; -- 1 day
        elseif UnitIndex == 3 then
          j = 259200; -- 3 days
        elseif UnitIndex == 4 then
          j = 604800; -- 1 week
        else
          return nil
        end
        if i < j then return true else return false end
      end
    end
  end
  return nil
end

function CheckPath(s)
  if string.find(s, '/.local/share/Trash/files/', 1, true) ~= nil then return true end
  if string.find(s, '/%.Trash%-%d+/files/') ~= nil then return true end
  if string.find(s, '/%.Trash/%d+/files/') ~= nil then return true end
  return nil
end
