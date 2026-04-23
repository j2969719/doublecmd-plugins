-- epubwdx.lua (cross-platform)
-- 2026.04.22
--[[
Getting some information from EPUB files (2/3).

See list of supported fields in table "fields".

Requires the `luazip` module.
- Windows:
  https://github.com/doublecmd/plugins/tree/master/wdx/fb2wdx
  (builded from https://github.com/mpeterv/luazip with Lua 5.1, thanks for the binary files to Alexander Koblov aka Alexx2000).
- Linux:
  Find LuaZip package, for example: "lua-zip" (Debian/Ubuntu and Debian/Ubuntu-based), "luazip5.1" (Arch Linux, from AUR).
  If not exists then try LuaRocks or try build:
    https://github.com/luaforge/luazip (used in Debian)
    https://github.com/mpeterv/luazip (fork with some fixes/changes)
    https://github.com/msva/luazip (fork too, used in Gentoo?)

Notes:
- All fields return text except "Publication date (year)",
  "Modification date (year)" and "Modification date (full)".
- The value of the "Modification date (full)" field depends on your regional
  settings and it may differ from the format in the "Files views" settings
   section: the main task is to search for files with plugins (see
  https://doublecmd.github.io/doc/en/findfiles.html#plugins).
- The value of the "Series: index" field can contain a signed number (zero and
  negative numbers are allowed) or a floating point number (with up to two
  digits of precision), so until another way is found, this field will return a
  string.
- Keep in mind that ISBN may be in URN format, without hyphens: this is considered
  valid and there doesn't seem to be an easy way to get a more readable look back.
- Keep in mind that sometimes file creators use "Publication" instead of "Original
  publication".

EntitiesToUTF8(): https://stackoverflow.com/a/26052539
]]

local r, zip = pcall(require, "zip")

local fields = {
{"EPUB version",                8},
{"Book title",                  8},
{"Creator: author",             8},
{"Creator: editor",             8},
{"Creator: translator",         8},
{"Creator (all)",               8},
{"Contributor: author",         8},
{"Contributor: editor",         8},
{"Contributor: translator",     8},
{"Contributor (all)",           8},
{"Subjects",                    8},
{"Description",                 8},
{"Publisher",                   8},
{"Rights",                      8},
{"Series",                      8},
{"Series: index",               8},
{"Language",                    8},
{"ISBN",                        8},
{"UID",                         8},
{"Publication (str)",           8},
{"Publication (year)",          1},
{"Original publication (str)",  8},
{"Original publication (year)", 1},
{"Modification (str)",          8},
{"Modification (year)",         1},
{"Modification (full)",        10}
}
local am = {}
local ar = {}
local sv, su, tz
local filename = ""

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2]
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="EPUB"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if r == false then return nil end
  if FieldIndex > #fields then return nil end
  local e
  if filename ~= FileName then
    e = string.lower(string.sub(FileName, string.len(FileName) - 4, -1))
    if e ~= ".epub" then return nil end
    am = {}
    GetMetadata(FileName)
    if #am == 0 then return nil end
    for i = 1, #fields do ar[i] = "" end
    GetFields()
    filename = FileName
  end
  if ar[FieldIndex + 1] == "" then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function GetMetadata(fn)
  local a = SysUtils.FileGetAttr(fn)
  if a == -1 then return nil end
  if (math.floor(a / 0x00000010) % 2 ~= 0) then return nil end
  local zf = zip.open(fn)
  if not zf then
    zf = zip.open(LazUtf8.ConvertEncoding(fn, "utf8", "default"))
    if not zf then return nil end
  end
  -- META-INF/container.xml
  local f = zf:open("META-INF/container.xml")
  if not f then return nil end
  local c = f:read("*a")
  f:close()
  if c == nil then return nil end
  local t = GetOPF(c)
  if t == nil then return nil end
  -- content.opf
  f = zf:open(t)
  if not f then return nil end
  c = f:read("*a")
  f:close()
  zf:close()
  local n1, n2 = string.find(c, "<package", 1, true)
  if n1 == nil then return nil end
  local n3
  n1, n3 = string.find(c, "<metadata", n2, true)
  if n1 == nil then return nil end
  n1 = string.find(c, "</metadata>", n3, true)
  if n1 == nil then return nil end
  t = string.sub(c, n2, n1)
  sv = string.match(t, 'version="([^"]+)"')
  su = string.match(t, 'unique%-identifier="([^"]+)"')
  local tn, ta, te, td
  local i = 1
  a = n1
  while true do
    n1, n2, tn, ta, te = string.find(c, "<([%w]+:?[%w]+)([^<]-)(%/?)>", n3)
    if (n1 == nil) or (n1 > a) then break end
    if te == "/" then
      n3 = n2
      td = ""
    else
      n3 = string.find(c, "</" .. tn .. ">", n2, true)
      td = string.sub(c, n2 + 1, n3 - 1)
    end
    am[i] = {tn, ta, td}
    i = i + 1
  end
end

function GetOPF(s)
  local n1, n2 = string.find(s, "<rootfiles>", 1, true)
  if n1 == nil then return nil end
  n1 = string.find(s, "<rootfile", n2, true)
  if n1 == nil then return nil end
  n2 = string.find(s, ">", n1, true)
  local t = string.sub(s, n1, n2)
  if string.len(t) > 0 then
    return string.match(t, 'full%-path="([^"]+)"')
  end
  return nil
end

function GetFields()
  -- EPUB version
  if sv == nil then return nil else ar[1] = sv end
  -- Book title
  local t = GetTData("dc:title")
  if t ~= nil then ar[2] = ClearString(t) end
  -- Creator: author(s), editor(s), translator(s)
  ar[3], ar[4], ar[5] = GetCreator()
  -- Creator (all)
  ar[6] = GetTDataList("dc:creator")
  -- Contributor: author(s), editor(s), translator(s)
  ar[7], ar[8], ar[9] = GetContributor()
  -- Creator (all)
  ar[10] = GetTDataList("dc:contributor")
  -- Subject/genres
  ar[11] = GetTDataList("dc:subject")
  -- Description
  t = GetTData("dc:description")
  if t ~= nil then ar[12] = ClearString(t) end
  -- Publisher
  t = GetTData("dc:publisher")
  if t ~= nil then ar[13] = ClearString(t) end
  -- Rights
  t = GetTData("dc:rights")
  if t ~= nil then ar[14] = ClearString(t) end
  -- Series / index
  t1, t2 = GetSeries()
  if t1 ~= nil then ar[15] = t1 end
  if t2 ~= nil then ar[16] = t2 end
  -- Language
  t = GetTData("dc:language")
  if t ~= nil then ar[17] = t end
  -- ISBN
  t = GetISBN()
  if t ~= nil then ar[18] = t end
  -- UID
  t = GetUID()
  if t ~= nil then ar[19] = t end
  -- Publication/Original publication/Modification date
  local t1, t2, t3, t4 = GetDate()
  local n
  if t1 ~= nil then
    ar[20] = t1
    n = string.match(t1, "^(%d+)")
    if n ~= nil then ar[21] = tonumber(n) end
  end
  if t2 ~= nil then
    ar[22] = t2
    n = string.match(t2, "^(%d+)")
    if n ~= nil then ar[23] = tonumber(n) end
  end
  if t3 ~= nil then
    ar[24] = t3
    n = string.match(t3, "^(%d+)")
    if n ~= nil then ar[25] = tonumber(n) end
    if t4 ~= nil then
      ar[26] = t4 * 10000000 + 116444736000000000
    end
  end
end

function GetTData(t)
  for i = 1, #am do
    if am[i][1] == t then return am[i][3] end
  end
  return nil
end

function GetTDataList(t)
  local sl = ""
  for i = 1, #am do
    if am[i][1] == t then
       sl =  sl .. "; " .. am[i][3]
    end
  end
  if string.len(sl) > 0 then sl = ClearString(string.sub(sl, 3, -1)) end
  return sl
end

function GetCreator()
  local sa, se, st = "", "", ""
  local r
  for i = 1, #am do
    if am[i][1] == "dc:creator" then
       r = string.match(am[i][2], 'opf:role="([^"]+)"')
       -- Workaround for some converters
       if r == nil then
         r = string.match(am[i][2], ':role="([^"]+)"')
       end
       if r == nil then
         sa = sa .. ", " .. am[i][3]
       elseif r == "aut" then
         sa = sa .. ", " .. am[i][3]
       elseif r == "edt" then
         se = se .. ", " .. am[i][3]
       elseif r == "trl" then
         st = st .. ", " .. am[i][3]
       end
    end
  end
  if string.len(sa) > 0 then sa = ClearString(string.sub(sa, 3, -1)) end
  if string.len(se) > 0 then se = ClearString(string.sub(se, 3, -1)) end
  if string.len(st) > 0 then st = ClearString(string.sub(st, 3, -1)) end
  return sa, se, st
end

function GetContributor()
  local sa, se, st = "", "", ""
  local r
  for i = 1, #am do
    if am[i][1] == "dc:contributor" then
       r = string.match(am[i][2], 'opf:role="([^"]+)"')
       -- Workaround for some converters
       if r == nil then
         r = string.match(am[i][2], ':role="([^"]+)"')
       end
       if r == "aut" then
         sa = sa .. ", " .. am[i][3]
       elseif r == "edt" then
         se = se .. ", " .. am[i][3]
       elseif r == "trl" then
         st = st .. ", " .. am[i][3]
       end
    end
  end
  if string.len(sa) > 0 then sa = ClearString(string.sub(sa, 3, -1)) end
  if string.len(se) > 0 then se = ClearString(string.sub(se, 3, -1)) end
  if string.len(st) > 0 then st = ClearString(string.sub(st, 3, -1)) end
  return sa, se, st
end

function GetSeries()
  local sn, si, id
  if string.sub(ar[1], 1, 1) == "3" then
    for i = 1, #am do
      if am[i][1] == "meta" then
        if string.find(am[i][2], 'property="belongs-to-collection"', 1, true) ~= nil then
          id = string.match(am[i][2], 'id="([^"]+)"')
          sn = am[i][3]
          break
        end
      end
    end
    if sn ~= nil then
      for i = 1, #am do
        if am[i][1] == "meta" then
          if string.find(am[i][2], 'property="group-position"', 1, true) ~= nil then
            if string.find(am[i][2], 'refines="#' .. id .. '"', 1, true) ~= nil then
              si = am[i][3]
              break
            end
          end
        end
      end
    end
  end
  if sn == nil then
    for i = 1, #am do
      if am[i][1] == "meta" then
        if string.find(am[i][2], 'name="calibre:series"', 1, true) ~= nil then
          sn = string.match(am[i][2], 'content="([^"]+)"')
          break
        end
      end
    end
    if sn ~= nil then
      for i = 1, #am do
        if am[i][1] == "meta" then
          if string.find(am[i][2], 'name="calibre:series_index"', 1, true) ~= nil then
            si = string.match(am[i][2], 'content="([^"]+)"')
            break
          end
        end
      end
    end
  end
  if sn ~= nil then sn = ClearString(sn) end
  return sn, si
end

function GetISBN()
  local r = ""
  local t
  if string.sub(ar[1], 1, 1) == "2" then
    for i = 1, #am do
      if am[i][1] == "dc:identifier" then
        if string.find(am[i][2], 'id="' .. su .. '"', 1, true) ~= nil then
          if string.find(am[i][2], 'opf:scheme="ISBN"', 1, true) ~= nil then
            r = r .. "; " .. am[i][3]
          end
        -- Workarounds
        else
          -- some files created/modified in Calibre
          if string.find(am[i][2], 'opf:scheme="ISBN"', 1, true) ~= nil then
            r = r .. "; " .. am[i][3]
          else
            -- some converters
            if string.find(am[i][2], ':scheme="ISBN"', 1, true) ~= nil then
              r = r .. "; " .. am[i][3]
            elseif string.sub(am[i][3], 1, 5) == "isbn:" then
              t = string.sub(am[i][3], 6, -1)
              r = r .. "; " .. t
            end
          end
        end
      end
    end
  else
    for i = 1, #am do
      if am[i][1] == "dc:identifier" then
        if string.find(am[i][2], 'id="' .. su .. '"', 1, true) ~= nil then
          if string.sub(am[i][3], 1, 9) == "urn:isbn:" then
            t = string.sub(am[i][3], 10, -1)
            r = r .. "; " .. t
          end
        end
      end
    end
  end
  if string.len(r) > 0 then r = string.sub(r, 3, -1) end
  return r
end

function GetUID()
  local r, s
  for i = 1, #am do
    if am[i][1] == "dc:identifier" then
      if string.find(am[i][2], 'id="' .. su .. '"', 1, true) ~= nil then
        if string.sub(am[i][3], 1, 9) == "urn:uuid:" then
          r = string.sub(am[i][3], 10, -1)
          break
        else
          s = am[i][3]
        end
      else
        if string.find(am[i][2], 'opf:scheme="UUID"', 1, true) ~= nil then
          if string.sub(am[i][3], 1, 9) == "urn:uuid:" then
            r = string.sub(am[i][3], 10, -1)
            break
          end
        else
          if string.find(am[i][2], 'opf:scheme="URI"', 1, true) ~= nil then
            if string.sub(am[i][3], 1, 9) == "urn:uuid:" then
              s = string.sub(am[i][3], 10, -1)
            end
          elseif string.find(am[i][2], ':scheme="URI"', 1, true) ~= nil then
            if string.sub(am[i][3], 1, 9) == "urn:uuid:" then
              s = string.sub(am[i][3], 10, -1)
            end
          end
        end
      end
    end
  end
  if r == nil then
    if s ~= nil then return s else return r end
  else
    return r
  end
end

function GetDate()
  local rp, ro, rm, rmi, s
  if string.sub(ar[1], 1, 1) == "2" then
    for i = 1, #am do
      if am[i][1] == "dc:date" then
        if am[i][2] == "" then
          if am[i][3] == "" then rp = nil else rp = am[i][3] end
        else
          if string.find(am[i][2], 'opf:event="publication"', 1, true) ~= nil then
            rp = am[i][3]
          elseif string.find(am[i][2], 'opf:event="original-publication"', 1, true) ~= nil then
            ro = am[i][3]
          elseif string.find(am[i][2], 'opf:event="modification"', 1, true) ~= nil then
            rm = am[i][3]
          -- Workaround for some converters
          elseif string.find(am[i][2], ':event="publication"', 1, true) ~= nil then
            rp = am[i][3]
          elseif string.find(am[i][2], ':event="original-publication"', 1, true) ~= nil then
            ro = am[i][3]
          elseif string.find(am[i][2], ':event="modification"', 1, true) ~= nil then
            rm = am[i][3]
          end
        end
      end
    end
  else
    for i = 1, #am do
      if am[i][1] == "dc:date" then
        if am[i][2] == "" then
          if am[i][3] == "" then rp = nil else rp = am[i][3] end
          break
        end
      end
    end
    for i = 1, #am do
      if am[i][1] == "meta" then
        if string.find(am[i][2], 'property="dcterms:modified"', 1, true) ~= nil then
          rm = am[i][3]
        else
          if string.find(am[i][2], 'property="dcterms:issued"', 1, true) ~= nil then
            ro = am[i][3]
          elseif string.find(am[i][2], 'property="dcterms:created"', 1, true) ~= nil then
            s = am[i][3]
          end
        end
      end
    end
    if ro == nil then ro = s end
  end
  if rp ~= nil then rp = ClearDateStr(rp) end
  if ro ~= nil then ro = ClearDateStr(ro) end
  if rm ~= nil then
    rmi = DateStrToUnixTime(rm)
    rm = ClearDateStr(rm)
  end
  return rp, ro, rm, rmi
end

function ClearString(s)
  local r = {
   ["&lt;"] = "<",
   ["&gt;"] = ">",
   ["&ndash;"] = "-",
   ["&mdash;"] = "-",
   ["&nbsp;"] = " ",
   ["&apos;"] = "'",
   ["&quot;"] = '"'
  }
  s = string.gsub(s, "&%a+;", function(e)
      return r[e] or e
    end)
  s = string.gsub(s, "&#%d+;", function(e)
      return EntitiesToUTF8(tonumber(string.sub(e, 3, -2)))
    end)
  s = string.gsub(s, "&#x%x+;", function(e)
      return EntitiesToUTF8(tonumber(string.sub(e, 4, -2), 16))
    end)
  s = string.gsub(s, "&amp;", "&")
  -- s = string.gsub(s, "\226\128\146", "-")
  -- s = string.gsub(s, "\226\128\147", "-")
  -- s = string.gsub(s, "\226\128\148", "-")
  -- s = string.gsub(s, "\226\128\149", "-")
  -- s = string.gsub(s, "\226\128\166", "...")
  s = string.gsub(s, "^%s+", "")
  s = string.gsub(s, "%s+$", "")
  return s
end

function EntitiesToUTF8(dec)
  local bytemarkers = {{0x7FF, 192}, {0xFFFF, 224}, {0x1FFFFF, 240}}
  if dec < 128 then return string.char(dec) end
  local cbs = {}
  for bytes, vals in ipairs(bytemarkers) do
    if dec <= vals[1] then
      for b = bytes + 1, 2, -1 do
        local mod = dec % 64
        dec = (dec - mod) / 64
        cbs[b] = string.char(128 + mod)
      end
      cbs[1] = string.char(vals[2] + dec)
      break
    end
  end
  return table.concat(cbs)
end

function ClearDateStr(s)
  local r, t1, t2, t3
  if string.find(s, "T", 1, true) ~= nil then
    if string.sub(s, -1, -1) == "Z" then
      r = string.gsub(s, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)T(%d%d):?(%d%d):?(%d%d)Z", "%1-%2-%3 %4:%5:%6Z")
    else
      t1, t2 = string.match(s, "(%d%d%d%d%-?%d%d%-?%d%dT%d%d:?%d%d:?%d%d)(.*)")
      if t2 == "" then
        r = string.gsub(t1, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)T(%d%d):?(%d%d):?(%d%d)", "%1-%2-%3 %4:%5:%6")
      else
        r = string.gsub(t1, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)T(%d%d):?(%d%d):?(%d%d)", "%1-%2-%3 %4:%5:%6")
        t3 = string.gsub(t2, "([%-%+]):?(%d%d):?(%d?%d?)", "%1%2:%3")
        if string.sub(t3, -1, -1) == ":" then
          r = r .. t3 .. "00"
        else
          r = r .. t3
        end
      end
    end
  else
    local n = string.len(s)
    if n <= 4 then
      r = string.match(s, "^(%d+)")
    elseif n <= 7 then
      r = string.gsub(s, "(%d%d%d%d)%-?(%d%d)", "%1-%2")
    elseif n >= 8 then
      r = string.gsub(s, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)", "%1-%2-%3")
    end
  end
  if r == nil then return s else return r end
end

function DateStrToUnixTime(s)
  -- Time zone
  if tz == nil then
    local ts = os.time()
    local ldt = os.time(os.date("*t", ts))
    local udt = os.time(os.date("!*t", ts))
    tz = ldt - udt
  end
  -- ISO 8601
  local dt = {}
  local tn, tb = 0, false
  local r, t1, t2, t3
--[[
  if (s == nil) or (s == "") then
    -- dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = 1970, 1, 1, 0, 0, 0
    -- return nil
]]
  if string.find(s, "T", 1, true) ~= nil then
    if string.sub(s, -1, -1) == "Z" then
      tb = true
      dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = string.match(s, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)T(%d%d):?(%d%d):?(%d%d)Z")
    else
      dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = string.match(s, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)T(%d%d):?(%d%d):?(%d%d).*")
      t1, t2, t3 = string.match(s, "([%-%+]):?(%d%d):?(%d?%d?)$")
      if t1 ~= nil then
        tb = true
        if t3 == "" then t3 = "0" end
        if t1 == "+" then
          tn = ((tonumber(t2) * 3600) + (tonumber(t3) * 60)) * -1
        else
          tn = (tonumber(t2) * 3600) + (tonumber(t3) * 60)
        end
      end
    end
  else
    local n = string.len(s)
    if n <= 4 then
      dt.year = string.match(s, "^(%d+)")
      dt.month, dt.day, dt.hour, dt.min, dt.sec = 1, 1, 0, 0, 0
    elseif n <= 7 then
      dt.year, dt.month = string.match(s, "(%d%d%d%d)%-?(%d%d)")
      dt.day, dt.hour, dt.min, dt.sec = 1, 0, 0, 0
    elseif n >= 8 then
      dt.year, dt.month, dt.day = string.match(s, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)")
      dt.hour, dt.min, dt.sec = 0, 0, 0
    else
      -- dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = 1970, 1, 1, 0, 0, 0
      return nil
    end
  end
  for k, v in pairs(dt) do
    if type(dt[k]) == "string" then dt[k] = tonumber(v) end
  end
  r = os.time(dt) + tn
  if tb == true then
    return r + tz
  else
    return r
  end
end
