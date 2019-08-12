-- djvuwdx.lua
-- 2019.08.12
--
-- Getting some information from DjVu files and searching text
-- Fields:
--  Pages
--    pages count (number, 0 if file is corrupted or other errors).
--  Chunk(s): bookmarks, hidden text, page annotation
--    boolean, true (if exists) or false.
--  Metadata
--    getting metadata fields (see table "meta").
--  Search in: hidden text, bookmarks, metadata
--    text search only (for "Find files" dialog!).
--
-- Script uses djvused from DjVuLibre http://djvu.sourceforge.net/

local fields = {
 {"Pages",     "", 1},
 {"Chunk(s)",  "bookmarks|hidden text|page annotation", 6},
 {"Metadata",  true, 8},
 {"Search in", "hidden text|bookmarks|metadata", 9}
}
local meta = {
 {"author",       "Author"},
 {"title",        "Title"},
 {"booktitle",    "BookTitle"},
 {"edition",      "Edition"},
 {"subject",      "Subject"},
 {"annote",       "Annotation"},
 {"keywords",     "Keywords"},
 {"note",         "Note"},
 {"journal",      "Journal"},
 {"number",       "Number"},
 {"volume",       "Volume"},
 {"chapter",      "Chapter"},
 {"series",       "Series"},
 {"year",         "Year"},
 {"institution",  "Institution"},
 {"organization", "Organization"},
 {"school",       "School"},
 {"publisher",    "Publisher"},
 {"creator",      "Creator"},
 {"producer",     "Producer"},
 {"creationdate", "Creation date"},
 {"moddate",      "Modification date"}
}
local md
local filename = ""

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    if fields[FieldIndex + 1][2] == true then
      local units = GetMetadataUnits()
      return fields[FieldIndex + 1][1], units, fields[FieldIndex + 1][3]
    else
      return fields[FieldIndex + 1][1], fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]
    end
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
  if FieldIndex == 0 then
    local h = io.popen('djvused -e n "' .. FileName .. '"')
    if not h then return nil end
    local num = h:read("*a")
    h:close()
    num = string.match(num, "(%d+)")
    if string.len(num) >= 1 then return num end
  elseif FieldIndex == 1 then
    local r = false
    if UnitIndex == 0 then
      local c = 1
      local h = io.popen('djvused -e dump "' .. FileName .. '"')
      if not h then return nil end
      for l in h:lines() do
        if (string.find(l, "^%s*NAVM %[", 1) ~= nil) then
          r = true
          break
        end
        c = c + 1
        if c > 5 then break end
      end
      h:close()
    else
      if UnitIndex == 1 then
        pat = "^%s*TXT[az] %["
      elseif UnitIndex == 2 then
        pat = "^%s*ANT[az] %["
      else
        return nil
      end
      local h = io.popen('djvused -e dump "' .. FileName .. '"')
      if not h then return nil end
      for l in h:lines() do
        if (string.find(l, pat, 1) ~= nil) then
          r = true
          break
        end
      end
      h:close()
    end
    return r
  elseif FieldIndex == 2 then
    if filename ~= FileName then
      local h = io.popen('djvused -e print-meta -u "' .. FileName .. '"')
      if not h then return nil end
      md = h:read("*a")
      h:close()
      filename = FileName
    end
    if string.len(md) > 1 then
      return GetMetadataField(md, meta[UnitIndex + 1][1])
    end
  elseif FieldIndex == 3 then
    local e
    if UnitIndex == 0 then
      e = "print-pure-txt"
    elseif UnitIndex == 1 then
      e = "print-outline"
    elseif UnitIndex == 2 then
      e = "print-meta"
    else
      return nil
    end
    local h = io.popen('djvused -e ' .. e .. ' -u "' .. FileName .. '"')
    if not h then return nil end
    local out = h:read("*a")
    h:close()
    if (UnitIndex == 2) and (string.len(out) > 1) then
      out = string.gsub(out, "[A-Za-z]+\t", "")
    end
    return out
  end
  return nil
end

function GetMetadataUnits()
  local s = ""
  for i = 1, #meta do s = s .. "|" .. meta[i][2] end
  return string.sub(s, 2, -1)
end

function GetMetadataField(s, f)
  local c = string.len(f)
  local r, t = "", ""
  for l in string.gmatch(s, "[^\r\n]+") do
    t = string.lower(string.sub(l, 1, c + 1))
    if t == f .. "\t" then
      r = string.sub(l, c + 3, -2)
      break
    end
  end
  if string.len(r) >= 1 then return r else return nil end
end
