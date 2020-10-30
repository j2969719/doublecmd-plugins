-- xcfinfowdx.lua (cross-platform)
-- 2020.10.30
--[[
Getting some info from XCF files (GIMP native image format).

See list of supported fields in table "fields".

hex2float() from
https://stackoverflow.com/questions/18886447/convert-signed-ieee-754-float-to-hexadecimal-representation
with small change.
]]

local fields = {
{"Version",               "", 1},
{"Minimal GIMP version",  "", 8},
{"Width",                 "", 2},
{"Height",                "", 2},
{"Horizontal resolution (ppi)", "", 3},
{"Vertical resolution (ppi)",   "", 3},
{"Color mode",            "", 8},
{"Precision",             "", 8},
{"ICC profile",           "", 8},
{"Compressed",            "", 8},
{"GIMP's comment",        "", 8}
}
local am = {
[0] = "0.99.16 (1997-12-15)",
[1] = "0.99.16 (1997-12-15)",
[2] = "1.3.10 (2002-11-07) or 1.3.21 (2003-10-06)",
[3] = "2.7.1 (2010-06-29)",
[4] = "2.10.0 (2018-04-27)",
[14] = "2.10.20 (2020-06-07)"
}
local ap4 = {
[0] = "8-bit gamma integer",
[1] = "16-bit gamma integer",
[2] = "32-bit linear integer",
[3] = "16-bit linear floating point",
[4] = "32-bit linear floating point"
}
local ap56 = {
[100] = "8-bit linear integer",
[150] = "8-bit gamma integer",
[200] = "16-bit linear integer",
[250] = "16-bit gamma integer",
[300] = "32-bit linear integer",
[350] = "32-bit gamma integer",
[400] = "16-bit linear floating point",
[450] = "16-bit gamma floating point",
[500] = "32-bit linear floating point",
[550] = "32-bit gamma floating point"
}
local ap7 = {
[100] = "8-bit linear integer",
[150] = "8-bit gamma integer",
[200] = "16-bit linear integer",
[250] = "16-bit gamma integer",
[300] = "32-bit linear integer",
[350] = "32-bit gamma integer",
[500] = "16-bit linear floating point",
[550] = "16-bit gamma floating point",
[600] = "32-bit linear floating point",
[650] = "32-bit gamma floating point",
[700] = "64-bit linear floating point",
[750] = "64-bit gamma floating point"
}
local ac = {
[0] = "No compression",
[1] = "RLE encoding",
[2] = "zlib compression",
[3] = "Some fractal compression"
}
local filename = ''
local ar = {}

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
  return 'EXT="XCF"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
    local e = SysUtils.ExtractFileExt(FileName)
    if e ~= '.xcf' then return nil end
    for i = 1, 11 do ar[i] = '' end
    if GetData(FileName) == nil then return nil end
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function GetData(f)
  local h = io.open(f, 'rb')
  if h == nil then return nil end
  local d = h:read(9)
  if d ~= 'gimp xcf ' then
    h:close()
    return nil
  end
  -- File type identification
  local i, t
  -- Version, 10:13
  d = h:read(4)
  if d == 'file' then
    ar[1] = 0
  else
    t = string.match(d, 'v(%d+)')
    if t == nil then ar[1] = '' else ar[1] = tonumber(t) end
  end
  -- Minimal GIMP version
  i = ar[1]
  if i <= 14 then
    if (i > 4) and (i < 14) then i = 4 end
    t = am[i]
    if t == nil then ar[2] = '' else ar[2] = t end
  else
    ar[2] = ''
  end
  h:seek('cur', 1)
  -- Width, 15:18
  d = h:read(4)
  ar[3] = BinToNum(d, 1, 4)
  -- Height, 19:22
  d = h:read(4)
  ar[4] = BinToNum(d, 1, 4)
  -- Color mode, 23:26
  d = h:read(4)
  i = BinToNum(d, 1, 4)
  if i == 0 then
    ar[7] = 'RGB color'
  elseif i == 1 then
    ar[7] = 'Grayscale'
  elseif i == 2 then
    ar[7] = 'Indexed color'
  else
    ar[7] = ''
  end
  -- Precision (high-bit depth), 27:30
  if ar[1] <= 3 then
    ar[8] = '8-bit gamma integer'
  else
    d = h:read(4)
    i = BinToNum(d, 1, 4)
    if ar[1] == 4 then
      t = ap4[i]
    elseif (ar[1] == 5) or (ar[1] == 6) then
      t = ap56[i]
    elseif ar[1] >= 7 then
      t = ap7[i]
    end
    if t == nil then ar[8] = '' else ar[8] = t end
  end
  -- Image properties
  local l, n
  while true do
    d = h:read(4)
    if d == nil then break end
    i = BinToNum(d, 1, 4)
    -- PROP_END
    if i == 0 then break end
    d = h:read(4)
    l = BinToNum(d, 1, 4)
    if i == 17 then
      -- PROP_COMPRESSION
      d = h:read(1)
      t = ac[string.byte(d, 1)]
      if t == nil then ar[10] = '' else ar[10] = t end
    elseif i == 19 then
      -- PROP_RESOLUTION
      d = h:read(8)
      ar[5] = hex2float(string.sub(d, 1, 4))
      ar[6] = hex2float(string.sub(d, 5, 8))
    elseif i == 21 then
      -- PROP_PARASITES
      d = h:read(l)
      GetFromParasites(d)
    else
      h:seek('cur', l)
    end
  end
  h:close()
  return true
end

function GetFromParasites(d)
  local t, c, n1, n2
  n1, n2 = string.find(d, 'icc-profile', 1, 1, true)
  if n1 == nil then
    ar[9] = 'GIMP built-in sRGB'
  else
    if string.byte(d, n2 + 1) == 0 then
      t = string.sub(d, n2 + 6, n2 + 9)
      c = BinToNum(t, 1, 4)
      t = string.sub(d, n2 + 10, n2 + c + 8)
      GetICCName(t)
    end
  end
  n1, n2 = string.find(d, 'gimp-comment', 1, 1, true)
  if n1 == nil then
    ar[11] = ''
  else
    t = string.sub(d, n2 + 6, n2 + 9)
    c = BinToNum(t, 1, 4)
    ar[11] = string.sub(d, n2 + 10, n2 + c + 8)
  end
end

function GetICCName(d)
  local t = string.sub(d, 129, 132)
  local tc = BinToNum(t, 1, 4)
  local nb = 132
  local i, l, di, dl
  while true do
    t = string.sub(d, nb + 1, nb + 12)
    i = BinToNum(t, 1, 4)
    if i == 0x64657363 then
      -- desc
      i = BinToNum(t, 5, 8)
      l = BinToNum(t, 9, 12)
      t = string.sub(d, i + 1, i + 4)
      if t == 'mluc' then
        di = BinToNum(d, i + 25, i + 28)
        dl = BinToNum(d, i + 21, i + 24)
        t = string.sub(d, i + di + 1, i + di + dl + 1)
        if math.mod(string.len(t), 2) ~= 0 then t = string.sub(t, 1, -2) end
        ar[9] = LazUtf8.ConvertEncoding(t, 'ucs2be', 'utf8')
      elseif t == 'desc' then
        dl = BinToNum(d, i + 9, i + 12)
        ar[9] = string.sub(d, i + 13, i + dl + 11)
      end
      break
    end
    tc = tc - 1
    if tc == 0 then break end
    nb = nb + 12
  end
end

function BinToNum(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = r .. string.format('%02x', string.byte(d, i))
  end
  return tonumber('0x' .. r)
end

function hex2float(c)
  local b1, b2, b3, b4 = string.byte(c, 1, 4)
  local sign = b1 > 0x7f
  local expo = (b1 % 0x80) * 0x2 + math.floor(b2 / 0x80)
  local mant = ((b2 % 0x80) * 0x100 + b3) * 0x100 + b4
  if sign then sign = -1 else sign = 1 end
  local n
  if (mant == 0) and (expo == 0) then
    n = sign * 0.0
  elseif expo == 0xff then
    if mant == 0 then
      n = sign * math.huge
    else
      n = 0.0/0.0
    end
  else
    n = sign * math.ldexp(1.0 + mant / 0x800000, expo - 0x7f)
  end
  return n
end
