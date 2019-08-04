-- djvuwdx.lua
-- 2019.08.03
--
-- Getting some information from DjVu files and searching text
-- Fields:
--  Pages             : pages count (number, 0 if file is corrupted or other errors);
--  Hidden text       : boolean, true (if exists) or false;
--  Outline/bookmarks : boolean, true (if exists) or false;
--  Page annotation   : boolean, true (if exists) or false;
--  Search in         : text search only (for "Find files" dialog),
--                      returns (if exists) hidden text, outline/bookmarks or metadata.
--
-- Script uses djvudump and djvused from DjVuLibre http://djvu.sourceforge.net/

local fields = {
 {"Pages",             "", 2},
 {"Hidden text",       "", 6},
 {"Outline/bookmarks", "", 6},
 {"Page annotation",   "", 6},
 {"Search in",         "hidden text|outline/bookmarks|metadata", 9}
}

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
  return '(EXT="DJVU")|(EXT="DJV")'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  local r = false
  if FieldIndex == 0 then
    local h = io.popen('djvused -e n "' .. FileName .. '"')
    if not h then return nil end
    local out = h:read("*a")
    h:close()
    out = string.match(out, '([^\r\n]+)')
    if string.len(out) >= 1 then return out end
  elseif FieldIndex == 1 then
    local h = io.popen('djvudump "' .. FileName .. '"')
    if not h then return nil end
    for l in h:lines() do
      if (string.find(l, ' TXT[az] %[', 1) ~= nil) then
        r = true
        break
      end
    end
    h:close()
    return r
  elseif FieldIndex == 2 then
    local c = 1
    local h = io.popen('djvudump "' .. FileName .. '"')
    if not h then return nil end
    for l in h:lines() do
      if (string.find(l, 'NAVM [ ', 1, true) ~= nil) then
        r = true
        break
      end
      c = c + 1
      if c > 4 then break end
    end
    h:close()
    return r
  elseif FieldIndex == 3 then
    local h = io.popen('djvudump "' .. FileName .. '"')
    if not h then return nil end
    for l in h:lines() do
      if (string.find(l, ' ANT[az] %[', 1) ~= nil) then
        r = true
        break
      end
    end
    h:close()
    return r
  elseif FieldIndex == 4 then
    local e
    if UnitIndex == 0 then
      e = 'print-pure-txt'
    elseif UnitIndex == 1 then
      e = 'print-outline'
    elseif FieldIndex == 2 then
      e = 'print-meta'
    else
      return nil
    end
    local h = io.popen('djvused -e ' .. e .. ' -u "' .. FileName .. '"')
    if not h then return nil end
    local out = h:read("*a")
    h:close()
    return out
  end
  return nil; -- invalid
end
