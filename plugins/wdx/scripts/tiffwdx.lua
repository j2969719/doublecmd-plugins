-- tiffwdx.lua (cross-platform)
-- 2023.02.04
--[[
Getting some information from TIFF files.
Supported fields: see table "fields".
]]

local fields = {
{"Multi-page image",   6},
{"Width",              1},
{"Height",             1},
{"X resolution",       1},
{"Y resolution",       1},
{"Color mode",         8},
{"Bits per component", 1},
{"Compression",        8},
{"Software",           8},
{"Date",               8}
}
local acm = {
[2] = "RGB",
[3] = "Palette-color",
[5] = "CMYK",
[6] = "YCbCr",
[8] = "CIE Lab"
} -- "Bilevel", "Indexed" and "Grayscale": see beloe
local acs = {
[0x0001] = "None",
[0x0002] = "CCITT G3 1D",
[0x0003] = "CCITT G3 2D",
[0x0004] = "CCITT G4",
[0x0005] = "LZW",
[0x0006] = "JPEG, obsolete",
[0x0007] = "JPEG",
[0x0008] = "Deflate (zlib), Adobe",
[0x0009] = "JBIG (black and white)",
[0x000a] = "JBIG (color, grayscale)",
[0x80b2] = "Deflate (PKZIP), obsolete",
[0x8005] = "PackBits",
[0x7ffe] = "NeXT RLE, 2-bit greyscale",
[0x8029] = "ThunderScan RLE, 4-bit",
[0x8080] = "RLE for line work (LW)",
[0x8081] = "RLE for high-resolution continuous-tone (HC)",
[0x8082] = "RLE for binary line work (BL)",
[0x80b3] = "Kodak DCS",
[0x8798] = "JPEG2000",
[0x8799] = "Nikon NEF Compressed",
[0x879b] = "JBIG2"
}
local ar = {"", "", "", "", "", "", "", "", "", ""}
local filename = ""

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2]
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return '(EXT="TIF")|(EXT="TIFF")'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if (e ~= '.tif') and (e ~= '.tiff') then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    local d = h:read(8)
    local t = string.sub(d, 1, 2)
    if (t == 'II') and (string.byte(d, 3) == 0x2a) then
      e = 'LE'
    elseif (t == 'MM') and (string.byte(d, 4) == 0x2a) then
      e = 'BE'
    else
      h:close()
      return nil
    end
    for j = 1, #ar do ar[j] = '' end
    -- Number of Directory Entries
    local n = BinToNum(d, 5, 8, e)
    h:seek('set', n)
    d = h:read(2)
    n = BinToNum(d, 1, 2, e)
    d = h:read(n * 12)
    local n1 = n * 12
    local i = 1
    local iBitsPerSample, iCompression, iPhotometricInt, iIndexed, iJPEGProc
    while true do
      t = BinToNum(d, i, i + 1, e)
      -- NewSubfileType
      if t == 0x00fe then
        n = BinToNum(d, i + 8, i + 11, e)
        if n == 1 then ar[1] = true end
      -- SubfileType
      elseif t == 0x00ff then
        n = BinToNum(d, i + 8, i + 11, e)
        if n == 3 then ar[1] = true end
      -- ImageWidth
      elseif t == 0x0100 then
        ar[2] = BinToNum(d, i + 8, i + 11, e)
      -- ImageLength
      elseif t == 0x0101 then
        ar[3] = BinToNum(d, i + 8, i + 11, e)
      -- BitsPerSample
      elseif t == 0x0102 then
        iBitsPerSample = BinToNum(d, i + 8, i + 11, e)
      -- Compression
      elseif t == 0x0103 then
        iCompression = BinToNum(d, i + 8, i + 11, e)
      -- PhotometricInterpretation
      elseif t == 0x0106 then
        iPhotometricInt = BinToNum(d, i + 8, i + 11, e)
      -- XResolution
      elseif t == 0x011a then
        n = BinToNum(d, i + 8, i + 11, e)
        h:seek('set', n)
        t = h:read(8)
        ar[4] = BinToNum(t, 1, 4, e) / BinToNum(t, 5, 8, e)
      -- YResolution
      elseif t == 0x011b then
        n = BinToNum(d, i + 8, i + 11, e)
        h:seek('set', n)
        t = h:read(8)
        ar[5] = BinToNum(t, 1, 4, e) / BinToNum(t, 5, 8, e)
      -- Software
      elseif t == 0x0131 then
        at = BinToNum(d, i + 4, i + 7, e)
        n = BinToNum(d, i + 8, i + 11, e)
        h:seek('set', n)
        ar[9] = h:read(at - 1)
      -- DateTime
      elseif t == 0x0132 then
        n = BinToNum(d, i + 8, i + 11, e)
        h:seek('set', n)
        ar[10] = h:read(19)
      -- Indexed
      elseif t == 0x015a then
        iIndexed = BinToNum(d, i + 8, i + 11, e)
      -- JPEGProc
      elseif t == 0x0200 then
        iJPEGProc = BinToNum(d, i + 8, i + 11, e)
      end
      i = i + 12
      if i > n1 then break end
    end
    h:close()
    if ar[1] == '' then ar[1] = false end
    -- Color mode
    if iIndexed == 1 then
      ar[6] = 'Indexed'
    else
      if (iPhotometricInt == 0) or (iPhotometricInt == 1) then
        if iBitsPerSample == 1 then
          ar[6] = 'Bilevel'
        else
          ar[6] = 'Grayscale'
        end
      else
        t = acm[iPhotometricInt]
        if t ~= nil then ar[6] = t end
      end
    end
    -- Compression
    if iCompression == 6 then
      if iJPEGProc == 1 then
        ar[8] = 'JPEG (baseline), obsolete'
      elseif iJPEGProc == 14 then
        ar[8] = 'JPEG (lossless), obsolete'
      end
    else
      t = acs[iCompression]
      if t ~= nil then ar[8] = t end
    end
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function BinToNum(d, n1, n2, e)
  local r = ''
  if e == 'LE' then
    for i = n1, n2 do r = string.format('%02x', string.byte(d, i)) .. r end
  else
    for i = n1, n2 do r = r .. string.format('%02x', string.byte(d, i)) end
  end
  return tonumber('0x' .. r)
end
