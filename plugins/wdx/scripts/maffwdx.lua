-- maffwdx.lua (cross-platform)
-- 2025.03.19
--[[
Getting some information from MAFF files.

Requeres the LuaZip module:
- Windows: zip.dll from https://github.com/doublecmd/plugins/tree/master/wdx/fb2wdx;
  Linux: find LuaZip package, for example:
  "lua-zip" (Debian/Ubuntu and Debian/Ubuntu-based), "luazip5.1" (Arch Linux, from AUR).
  If not exists then try LuaRocks or try build
  https://github.com/luaforge/luazip (used in Debian?)
  https://github.com/mpeterv/luazip (fork with some fixes/changes)

EntitiesToUTF8(): https://stackoverflow.com/a/26052539
]]

local r, zip = pcall(require, "zip")

local fields = {
{"Root folder",   "", 8},
{"Original URL",  "", 8},
{"Title",         "", 8},
{"Date (string)", "", 8},
{"Date",          "year|month|day|hour|min|sec", 1}
}
local aM = {
["Jan"] = "01",
["Feb"] = "02",
["Mar"] = "03",
["Apr"] = "04",
["May"] = "05",
["Jun"] = "06",
["Jul"] = "07",
["Aug"] = "08",
["Sep"] = "09",
["Oct"] = "10",
["Nov"] = "11",
["Dec"] = "12"
}
local aU = {
["&lt;"] = "<",
["&gt;"] = ">",
["&ndash;"] = "-",
["&mdash;"] = "-",
["&nbsp;"] = " ",
["&apos;"] = "'",
["&quot;"] = '"'
}

local aR = {}
local filename = ""

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="MAFF"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 4 then return nil end
  local e
  if filename ~= FileName then
    e = string.lower(string.sub(FileName, string.len(FileName) - 4, -1))
    if e ~= ".maff" then return nil end
    if GetData(FileName) == nil then return nil end
    filename = FileName
  end
  if FieldIndex < 4 then
    return aR[FieldIndex + 1]
  else
    if UnitIndex == 0 then
      e = string.sub(aR[4], 1, 4)
    elseif UnitIndex == 1 then
      e = string.sub(aR[4], 6, 7)
    elseif UnitIndex == 2 then
      e = string.sub(aR[4], 9, 10)
    elseif UnitIndex == 3 then
      e = string.sub(aR[4], 12, 13)
    elseif UnitIndex == 4 then
      e = string.sub(aR[4], 15, 16)
    elseif UnitIndex == 5 then
      e = string.sub(aR[4], 18, 19)
    else
      return nil
    end
    return tonumber(e, 10)
  end
  return nil
end

function GetData(fn)
  if r == false then return nil end
  local zf = zip.open(fn)
  if not zf then
    zf = zip.open(LazUtf8.ConvertEncoding(fn, "utf8", "default"))
    if not zf then return nil end
  end
  local n, f, c
  for fz in zf:files() do
    n = string.find(fz.filename, "/", 1, true)
    if string.sub(fz.filename, n, -1) == "/index.rdf" then
      aR[1] = string.sub(fz.filename, 1, n - 1)
      f = zf:open(fz.filename)
      if not f then break end
      c = f:read("*a")
      f:close()
      break
    end
  end
  zf:close()
  if c == nil then return nil end
  local t = GetFileData(c, '<MAF:originalurl RDF:resource="')
  aR[2] = ClearString(t)
  t = GetFileData(c, '<MAF:title RDF:resource="')
  aR[3] = ClearString(t)
  t = GetFileData(c, '<MAF:archivetime RDF:resource="')
  t = string.gsub(t, "[A-Za-z]+", function(m)
      return aM[m] or m
    end)
  aR[4] = string.gsub(t, "^[A-Za-z]+[^%d]+(%d%d)%s(%d%d)%s(%d%d%d%d)%s([0-9:]+).+$", "%3.%2.%1 %4")
  return true
end

function GetFileData(s, t)
  local n1, n2 = string.find(s, t, 1, true)
  if n1 == nil then return "" end
  n1 = string.find(s, '"/>', n2, true)
  s = string.sub(s, n2 + 1, n1 - 1)
  if string.len(s) > 0 then return s end
  return ""
end

function ClearString(s)
  s = string.gsub(s, "&%a+;", function(e)
      return aU[e] or e
    end)
  s = string.gsub(s, "&#%d+;", function(e)
      return EntitiesToUTF8(tonumber(string.sub(e, 3, -2)))
    end)
  s = string.gsub(s, "&#x%x+;", function(e)
      return EntitiesToUTF8(tonumber(string.sub(e, 4, -2), 16))
    end)
  s = string.gsub(s, "&amp;", "&")
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
