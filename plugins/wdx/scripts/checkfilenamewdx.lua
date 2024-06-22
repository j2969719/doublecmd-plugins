-- checkfilenamewdx.lua (cross-platform)
-- 2024.06.22
--[[
Save as UTF-8 without BOM!

Fields:

1) Latin and additional characters
3) Latin, cyrillic and additional characters
returns all characters except:
- latin alphabet (1) or latin + cyrillic (3);
- numbers;
- space (only 0x20);
- hyphen, comma, dot, round and square brackets, single quotation mark, underscore: -,.!()[]'_

2) Latin and additional characters: count
4) Latin, cyrillic and additional characters: count
like 1) and 3), but returns only the number of characters.

5) File naming in Windows
Check limitations and recommendations, list:
- reserved characters;
- forbidden characters: numerical codes are in the range 0 - 31 (0x00 - 0x1f);
- reserved names (name or base name);
- name with a space in the end;
- name with a dot in the end.
Returns "Good" if all is good or string with error message(s).

6) Path length:
- characters;
- bytes;
- characters (relative);
- bytes (relative).

"characters (relative)" and "bytes (relative)" for Unix-like OS:
extracting a path which be relative to
  /dev/ad
  /dev/da
  /dev/fd
  /dev/hd
  /dev/nvme
  /dev/sd
  /dev/sr
i.e. relative to mount points.
Result will not have a path delimiter in the beginning!
In Windows this units returns path without "[drive]:\".
Note: In Unix-like script uses /etc/mtab.

7) Path length: chars = bytes
Returns true if "path length in characters" = "path length in bytes".

Notes about a path length limitation in Windows:
1. MAX_PATH
In some cases the maximum length for a path is MAX_PATH, which is defined as 260 characters:
  [drive]:\[some 256-character path string]<NUL>
So, if you want to find (or highlight with help "Colors > File types") a file with path more
than MAX_PATH, you should use:
  Win: field "Path length: characters" and condition "> 259".
  Unix-like: field "Path length: characters (relative)" and condition "> 256"
2. An extended-length path
A maximum total path length is 32767 characters, but with the "\\?\" prefix:
the "\\?\" prefix may be expanded to a longer string by the system at run time, so you
can not use "32767" or "32767 - 1" in conditions.
---------------------

Also see caseduplwdx.lua, it can be useful too.
]]

local cyr = {
"А", "Б", "В", "Г", "Д", "Е", "Ё", "Ж", "З", "И", "Й",
"К", "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф",
"Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я",
"а", "б", "в", "г", "д", "е", "ё", "ж", "з", "и", "й",
"к", "л", "м", "н", "о", "п", "р", "с", "т", "у", "ф",
"х", "ц", "ч", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я"
}
local rname = {
"con", "prn", "aux", "nul",
"com1", "com2", "com3",
"com4", "com5", "com6",
"com7", "com8", "com9",
"lpt1", "lpt2", "lpt3",
"lpt4", "lpt5", "lpt6",
"lpt7", "lpt8", "lpt9"
}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Latin and additional characters", "", 8
  elseif FieldIndex == 1 then
    return "Latin and additional characters: count", "", 1
  elseif FieldIndex == 2 then
    return "Latin, cyrillic and additional characters", "", 8
  elseif FieldIndex == 3 then
    return "Latin, cyrillic and additional characters: count", "", 1
  elseif FieldIndex == 4 then
    return "File naming in Windows", "", 8
  elseif FieldIndex == 5 then
    return "Path length", "characters|bytes|characters (relative)|bytes (relative)", 1
  elseif FieldIndex == 6 then
    return "Path length: chars = bytes", "", 6
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 6 then return nil end
  --if flags == 1 then return nil end; -- Исключаем вывод для диалога свойств (CONTENT_DELAYIFSLOW)
  local t = string.sub(FileName, -3, -1)
  if (t == '/..') or (t == '\\..') then return nil end
  t = string.sub(t, 2, 3)
  if (t == '/.') or (t == '\\.') then return nil end
  if (FieldIndex >= 0) and (FieldIndex <= 3) then
    local fn = SysUtils.ExtractFileName(FileName)
    local afn = GetTableUTF8Char(fn)
    local afc = {}
    local n = 1
    table.sort(afn)
    for i = 1, #afn do
      if afn[i] ~= afn[i + 1] then
        afc[n] = afn[i]
        n = n + 1
      end
    end
    for i = 1, #afc do
      if string.len(afc[i]) == 1 then
        n = string.byte(afc[i])
        -- 0-9, A-Z, a-z
        if (n > 47) and (n < 58) then
          afc[i] = ""
        elseif (n > 64) and (n < 91) then
          afc[i] = ""
        elseif (n > 96) and (n < 123) then
          afc[i] = ""
        else
          -- Additional characters
          if (afc[i] == " ") or (afc[i] == "-") or (afc[i] == ",") or (afc[i] == ".") or (afc[i] == "!") or (afc[i] == "(") or (afc[i] == ")") or (afc[i] == "[") or (afc[i] == "]") or (afc[i] == "'") or (afc[i] == "_") then
            afc[i] = ""
          end
        end
      end
    end
    if (FieldIndex == 2) or (FieldIndex == 3) then
      -- Cyrillic
      for i = 1, #afc do
        if string.len(afc[i]) > 1 then
          for j = 1, #cyr do
            if afc[i] == cyr[j] then
              afc[i] = ""
              break
            end
          end
        end
      end
    end
    if math.fmod(FieldIndex, 2) == 0 then
      return table.concat(afc)
    else
      return LazUtf8.Length(table.concat(afc))
    end
  elseif FieldIndex == 4 then
    local fn = SysUtils.ExtractFileName(FileName)
    local afn = GetTableUTF8Char(fn)
    local r = ""
    for i = 1, #afn do
      if string.len(afn[i]) == 1 then
        if (afn[i] == '<') or (afn[i] == '>') or (afn[i] == ':') or (afn[i] == '"') or (afn[i] == '|') or (afn[i] == '?') or (afn[i] == '*') or (afn[i] == '/') or (afn[i] == '\\') then
          r = 'Reserved characters!'
          break
        end
      end
    end
    for i = 1, #afn do
      if string.len(afn[i]) == 1 then
        if string.byte(afn[i]) < 32 then
          r = r .. ' Forbidden characters!'
          break
        end
      end
    end
    fn = string.lower(fn)
    if string.find(fn, '.', 1, true) == nil then
      for i = 1, #rname do
        if fn == rname[i] then
          r = r .. ' Reserved names!'
          break
        end
      end
    else
      local nm = string.sub(fn, 1, string.len(fn) - string.len(SysUtils.ExtractFileExt(fn)))
      if nm == '' then nm = fn end
      for i = 1, #rname do
        if nm == rname[i] then
          r = r .. ' Reserved names!'
          break
        end
      end
    end
    if afn[#afn] == ' ' then
      r = r .. ' Space in the end!'
    elseif afn[#afn] == '.' then
      r = r .. ' Dot in the end!'
    end
    if string.sub(r, 1, 1) == ' ' then r = string.gsub(r, '^%s+', '') end
    if string.len(r) == 0 then return "Good" else return r end
  elseif FieldIndex == 5 then
    if UnitIndex == 0 then
      return LazUtf8.Length(FileName)
    elseif UnitIndex == 1 then
      return string.len(FileName)
    else
      local s
      if SysUtils.PathDelim == '\\' then
        if string.sub(FileName, 2, 3) == ':\\' then
          s = string.sub(FileName, 4, -1)
        else
          return nil
        end
      else
        s = GetRelativeToDev(FileName)
        if s == nil then return nil end
      end
      if UnitIndex == 2 then
        return LazUtf8.Length(s)
      elseif UnitIndex == 3 then
        return string.len(s)
      end
    end
  elseif FieldIndex == 6 then
    local sl = string.len(FileName)
    local ll = LazUtf8.Length(FileName)
    if sl == ll then
      return true
    elseif sl ~= ll then
      return false
    end
  end
  return nil
end

function GetTableUTF8Char(s)
  local l = string.len(s)
  local i, j = 1, 1
  local t = {}
  local n
  -- rfc3629
  while true do
    if i > l then break end
    n = string.byte(s, i)
    if (n >= 0) and (n <= 127) then
      t[j] = string.sub(s, i, i)
      i = i + 1
    elseif (n >= 194) and (n <= 223) then
      t[j] = string.sub(s, i, i + 1)
      i = i + 2
    elseif (n >= 224) and (n <= 239) then
      t[j] = string.sub(s, i, i + 2)
      i = i + 3
    elseif (n >= 240) and (n <= 244) then
      t[j] = string.sub(s, i, i + 3)
      i = i + 4
    end
    j = j + 1
  end
  return t
end

function GetRelativeToDev(s)
  local r, t, rt
  local am = {}
  local cm = 1
  local h = io.open('/etc/mtab', 'r')
  if not h then return nil end
  for l in h:lines() do
    t = string.sub(l, 1, 7)
    if (t == '/dev/ad') or (t == '/dev/da') or (t == '/dev/fd') or (t == '/dev/hd') or (t == '/dev/sd') or (t == '/dev/sr') or (string.sub(l, 1, 9) == '/dev/nvme') then
      t = string.match(l, '^[^ ]+ ([^ ]+) ')
      if t == '/' then
        rt = '/'
      else
        if string.find(t, '\\', 1, true) ~= nil then
          t = string.gsub(t, '\\([0-7][0-7][0-7])',
                function(d) return string.char(tonumber(d, 8)) end)
        end
        am[cm] = t
        cm = cm + 1
      end
    end
  end
  h:close()
  if #am == 0 then return nil end
  for i = 1, #am do
    cm = string.len(am[i])
    t = string.sub(s, 1, cm + 1)
    if t == am[i] .. '/' then
      r = string.sub(s, cm + 2, -1)
      break
    end
  end
  if (r == nil) and (rt ~= nil) then r = string.sub(s, 2, -1) end
  return r
end
