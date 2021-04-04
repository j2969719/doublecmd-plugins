-- msofficeoldwdx.lua
-- 2021.04.04
--[[
Getting some information from Microsoft Office 97-2003 files: *.doc, *.xls, *.ppt.

Supported fields: see table "fields".
Plugin returns date/time as string (will used local time).

Plugin uses the GNOME Structured File Library (GSF):
- Debian/Ubuntu and based on them: libgsf-bin;
- Arch Linux: libgsf.
"gsf" should be >= 1.14.1.

P.S. For Windows use CDocProp:
http://totalcmd.net/plugring/CDocProp.html
http://wincmd.ru/plugring/CDocProp.html
]]

local fields = {
{"Title",                     "dc:title", 8},
{"Subject",                   "dc:subject", 8},
{"Description",               "dc:description", 8},
{"Category",                  "gsf:category", 8},
{"Keywords",                  "dc:keywords", 8},
{"Creator",                   "dc:creator", 8},
{"Company/organization",      "dc:publisher", 8},
{"Revision count",            "meta:editing-cycles", 1},
{"Creation date",             "meta:creation-date", 8},
{"Modification date",         "dc:date", 8},
{"Print_date",                "gsf:last-printed", 8},
{"Last saved by",             "gsf:last-saved-by", 8},
{"Last printed by",           "meta:printed-by", 8},
{"Application",               "meta:generator", 8},
{"Security",                  "gsf:security", 8},
{"Template",                  "meta:template", 8},
{"Count of tables",           "gsf:table-count", 1},
{"Count of images",           "gsf:image-count", 1},
{"Count of objects",          "gsf:object-count", 1},
{"Count of notes",            "gsf:note-count", 1},
{"Count of pages",            "gsf:page-count", 1},
{"Count of paragraphs",       "gsf:paragraph-count", 1},
{"Count of lines",            "gsf:line-count", 1},
{"Count of words",            "gsf:word-count", 1},
{"Count of characters",       "gsf:character-count", 1},
{"Count of pages (XLS)",      "gsf:spreadsheet-count", 1},
{"Count of cells (XLS)",      "gsf:cell-count", 1},
{"Type of presentation (PPT)","gsf:presentation-format", 8},
{"Count of slides (PPT)",     "gsf:slide-count", 1},
{"Count of hidden slides",    "gsf:hidden-slide-count", 1},
{"Count of multimedia clips", "gsf:MM-clip-count", 1}
}
local asec = {
[0] = "None",
[1] = "Password protected",
[2] = "Read-only recommended",
[3] = "Read-only enforced",
[4] = "Locked for annotations"
}
local ar = {}
local filename = ""
local props = ""
local toff

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][3]
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return '(EXT="DOC")|(EXT="XLS")|(EXT="PPT")'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) or (math.floor(at / 0x00000400) % 2 ~= 0) then return nil end
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if (e ~= '.doc') and (e ~= '.xls') and (e ~= '.ppt') then return nil end
    if props == '' then
      for i = 1, #fields do
        props = props .. fields[i][2] .. ' '
      end
      props = props .. '2>&1'
    end
    if toff == nil then toff = GetTimeOffset() end
    local c = 1
    local n
    local h = io.popen('gsf props "' .. FileName .. '" ' .. props)
    if h == nil then return nil end
    for l in h:lines() do
      n = string.find(l, '=', 1, true)
      if n == nil then
        ar[c] = ''
      else
        if string.sub(l, n + 2, n + 2) == '"' then
          ar[c] = string.sub(l, n + 3, -2)
        else
          e = string.sub(l, n + 2, -1)
          if string.find(e, '[^0-9]', 1, false) == nil then
            ar[c] = e
          else
            -- Looks like date/time
            ar[c] = FormatDate(e)
          end
        end
      end
      c = c + 1
    end
    h:close()
    for i = 1, #ar do
      if ar[i] ~= '' then
        t = ar[i]
        if fields[i][3] == 1 then
          ar[i] = tonumber(t)
        elseif fields[i][3] == 8 then
          -- Replace octal \nnn on characters
          ar[i] = string.gsub(t, '(\\%d%d%d)', function (x) return octal2char(x) end)
        end
      end
    end
    --"Security" as text:
    e = asec[ar[15]]
    if e == nil then ar[15] = '' else ar[15] = e end
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function GetTimeOffset()
  local sec = os.time()
  local udt = os.date("!*t", sec)
  local ldt = os.date("*t", sec)
  return os.difftime(os.time(ldt), os.time(udt))
end

function FormatDate(s)
  local dt = {}
  dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = string.match(s, '(%d%d%d%d)%-(%d%d)%-(%d%d)T(%d%d):(%d%d):(%d%d)Z')
  if dt.year == nil then
    return s
  else
    return os.date('%Y.%m.%d %H:%M:%S', os.time(dt) + toff)
  end
end

function octal2char(s)
  local r = 0
  for i = 2, 4 do
    r = r * 8 + tonumber(string.sub(s, i, i))
  end
  return string.char(r)
end
