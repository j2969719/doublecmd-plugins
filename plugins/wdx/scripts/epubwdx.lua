-- epubwdx.lua (cross-platform)
-- 2026.04.14
--[[

EntitiesToUTF8(): https://stackoverflow.com/a/26052539
]]

local r, zip = pcall(require, "zip")

local fields = {
{"EPUB version",       8},
{"Book title",         8},
{"Author(s)",          8},
{"Translator(s)",      8},
{"Contributor(s)",     8},
{"Genres",             8},
{"Description",        8},
{"Language",           8},
{"Publication date",   8},
{"Modification date",  8},
{"Publisher",          8},
{"Rights",             8},
{"UID",                8}
}
local bd, bdp, tz
local ar = {}
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
    bd = GetMetadata(FileName)
    if bd == nil then return nil end
    for i = 1, #ar do ar[i] = "" end
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
  local n = SysUtils.FileGetAttr(fn)
  if n == -1 then return nil end
  if (math.floor(n / 0x00000010) % 2 ~= 0) then return nil end
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
  local c = f:read("*a")
  f:close()
  zf:close()
  local n1, n2 = string.find(c, "<package", 1, true)
  if n1 == nil then return nil end
  n1, n = string.find(c, "<metadata", n2, true)
  if n1 == nil then return nil end
  bdp = string.sub(c, n2, n)
  n1 = string.find(c, "</metadata>", n, true)
  if n1 == nil then return nil end
  return string.sub(c, n, n1)
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
  local t = string.match(bdp, 'version="([^"]+)"')
  if t == nil then return nil else ar[1] = t end
  -- Book title
  t = GetTagData("dc:title")
  if t == nil then ar[2] = "" else ar[2] = ClearString(t) end
  -- Creator: Author(s), Translator(s)
  ar[3], ar[4] = GetAuthors()
  -- Contributor
  ar[5] = GetContributors()
  -- Subject: Genres
  ar[6] = GetGenres()
  -- Description
  t = GetTagData("dc:description")
  if t == nil then ar[7] = "" else ar[7] = ClearString(t) end
  -- Language
  t = GetTagData("dc:language")
  if t == nil then ar[8] = "" else ar[8] = t end
   -- Publication/Modification date
   local t1, t2 = GetDate()
   ar[9] = os.date("%Y.%m.%d %H:%M:%S", t1)
   ar[10] = os.date("%Y.%m.%d %H:%M:%S", t2)



-- = UnixTime * 10000000 + 116444736000000000



  -- Publisher
  t = GetTagData("dc:publisher")
  if t == nil then ar[11] = "" else ar[11] = ClearString(t) end
  -- Rights
  t = GetTagData("dc:rights")
  if t == nil then ar[12] = "" else ar[12] = ClearString(t) end
  -- UID
  t = GetUID()
  if t == nil then ar[13] = "" else ar[13] = t end
end

function GetAuthors()
  local sa, st, n3 = "", "", 1
  local n1, n2, r, t
  while true do
    n1 = string.find(bd, "<dc:creator", n3, true)
    if n1 == nil then break end
    n2 = string.find(bd, ">", n1, true)
    n3 = string.find(bd, "</dc:creator>", n2, true)
    t = string.sub(bd, n1, n2)
    r = string.match(t, 'opf:role="([^"]+)"')
    if r == nil then
      t = string.sub(bd, n2 + 1, n3 - 1)
      sa = sa .. ", " .. t
    elseif r == "aut" then
      t = string.sub(bd, n2 + 1, n3 - 1)
      sa = sa .. ", " .. t
    elseif r == "trl" then
      t = string.sub(bd, n2 + 1, n3 - 1)
      st = st .. ", " .. t
    end
  end
  if string.len(sa) > 0 then sa = ClearString(string.sub(sa, 3, -1)) end
  if string.len(st) > 0 then st = ClearString(string.sub(st, 3, -1)) end
  return sa, st
end

function GetContributors()
  local sc, n3 = "", 1
  local n1, n2, t
  while true do
    n1 = string.find(bd, "<dc:contributor", n3, true)
    if n1 == nil then break end
    n2 = string.find(bd, ">", n1, true)
    n3 = string.find(bd, "</dc:contributor>", n2, true)
    t = string.sub(bd, n2 + 1, n3 - 1)
    sc = sc .. "; " .. t
  end
  if string.len(sc) > 0 then sc = ClearString(string.sub(sc, 3, -1)) end
  return sc
end

function GetGenres()
  local sg, n3 = "", 1
  local n1, n2, t
  while true do
    n1 = string.find(bd, "<dc:subject", n3, true)
    if n1 == nil then break end
    n2 = string.find(bd, ">", n1, true)
    n3 = string.find(bd, "</dc:subject>", n2, true)
    if n3 == nil then break end
    t = string.sub(bd, n2 + 1, n3 - 1)
    sg = sg .. "; " .. t
  end
  if string.len(sg) > 0 then sg = ClearString(string.sub(sg, 3, -1)) end
  return sg
end

function GetDate()
  local n3 = 1
  local n1, n2, rp, rm, t
  if string.sub(ar[1], 1, 1) == "2" then
    while true do
      n1 = string.find(bd, "<dc:date", n3, true)
      if n1 == nil then break end
      n2 = string.find(bd, ">", n1, true)
      n3 = string.find(bd, "</dc:date>", n2, true)
      if string.sub(bd, n1, n2) == "<dc:date>" then
        rp = string.sub(bd, n2 + 1, n3 - 1)
        if rp == "" then rp = nil end
      else
        t = string.sub(bd, n1, n2)
        if string.find(t, 'opf:event="publication"', 1, true) ~= nil then
          rp = string.sub(bd, n2 + 1, n3 - 1)
        elseif string.find(t, 'opf:event="original-publication"', 1, true) ~= nil then
          rp = string.sub(bd, n2 + 1, n3 - 1)
        elseif string.find(t, 'opf:event="modification"', 1, true) ~= nil then
          rm = string.sub(bd, n2 + 1, n3 - 1)
        end
      end
    end
  else
    while true do
      n1 = string.find(bd, "<dc:date", n3, true)
      if n1 == nil then break end
      n2 = string.find(bd, ">", n1, true)
      n3 = string.find(bd, "</dc:date>", n2, true)
      if string.sub(bd, n1, n2) == "<dc:date>" then
        rp = string.sub(bd, n2 + 1, n3 - 1)
        if rp == "" then rp = nil end
        break
      end
    end
    while true do
      n1 = string.find(bd, "<meta", n3, true)
      if n1 == nil then break end
      n2 = string.find(bd, ">", n1, true)
      n3 = string.find(bd, "</meta>", n2, true)
      t = string.sub(bd, n1, n2)
      if string.find(t, 'property="dcterms:modified"', 1, true) ~= nil then
        rm = string.sub(bd, n2 + 1, n3 - 1)
        break
      end
    end
  end
  n1 = FormatDate(rp)
  n2 = FormatDate(rm)
  return n1, n2
end

function GetUID()
  local ui = string.match(bdp, 'unique%-identifier="([^"]+)"')
  local n3 = 1
  local n1, n2, r, t
  while true do
    n1 = string.find(bd, "<dc:identifier", n3, true)
    if n1 == nil then break end
    n2 = string.find(bd, ">", n1, true)
    n3 = string.find(bd, "</dc:identifier>", n2, true)
    t = string.sub(bd, n1, n2)
    if string.find(t, 'id="' .. ui .. '"', 1, true) ~= nil then
      r = string.sub(bd, n2 + 1, n3 - 1)
      break
    end
  end
  return r
end

function GetTagData(t)
  local n1, n2 = string.find(bd, "<" .. t, 1, true)
  if n1 == nil then return nil end
  n1 = string.find(bd, ">", n2, true)
  n2 = string.find(bd, "</" .. t .. ">", n1, true)
  if n2 == nil then return nil end
  local s = string.sub(bd, n1 + 1, n2 - 1)
  if string.len(s) > 0 then return s else return nil end
end

function GetTagAttr(t)
  local n1 = string.find(bd, "<" .. t, 1, true)
  if n1 == nil then return nil end
  local n2 = string.find(bd, ">", n1, true)
  local s = string.sub(bd, n1, n2)
  if string.len(s) > 0 then return s else return nil end
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

function FormatDate(s)
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
  if s == nil then
    dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = 1970, 1, 1, 0, 0, 0
  else
    local n = string.len(s)
    if n == 4 then
      dt.year = string.match(s, "^(%d+)")
      dt.month, dt.day, dt.hour, dt.min, dt.sec = 1, 1, 0, 0, 0
    elseif n == 8 then
      dt.year, dt.month, dt.day = string.match(s, "(%d%d%d%d)(%d%d)(%d%d)")
    elseif (n == 10) and (string.sub(s, 5, 5) == "-") then
      dt.year, dt.month, dt.day = string.match(s, "(%d%d%d%d)%-(%d%d)%-(%d%d)")
    else
      tb = true
      if string.sub(s, -1, -1) == "Z" then
        dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = string.match(s, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)T(%d%d):?(%d%d):?(%d%d)Z")
      else
        dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = string.match(s, "(%d%d%d%d)%-?(%d%d)%-?(%d%d)T(%d%d):?(%d%d):?(%d%d).*")
        t1, t2, t3 = string.match(s, "([%-%+]):?(%d%d):?(%d?%d?)$")
        if t3 == "" then t3 = "0" end
        if t1 == "+" then
          tn = ((tonumber(t2) * 3600) + (tonumber(t3) * 60)) * -1
        else
          tn = (tonumber(t2) * 3600) + (tonumber(t3) * 60)
        end
      end
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
