-- psdwdx.lua (cross-platform)
-- 2023.02.03
--[[
Getting some information from PSD files.
Supported fields: see table "fields".
]]

local fields = {
{"Width",              1},
{"Height",             1},
{"Bits per channel",   1},
{"Color mode",         8},
{"Number of channels", 1}
}
local acm = {
[0] = "Bitmap",
[1] = "Grayscale",
[2] = "Indexed",
[3] = "RGB",
[4] = "CMYK",
[7] = "Multichannel",
[8] = "Duotone",
[9] = "Lab"
}
local ar = {}
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
  return 'EXT="PSD"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if e ~= '.psd' then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    local d = h:read(26)
    h:close()
    if string.sub(d, 1, 4) ~= '8BPS' then return nil end
    local n = BinToNumBE(d, 5, 6)
    if (n ~= 1) and (n ~= 2) then return nil end
    -- Width
    ar[1] = BinToNumBE(d, 19, 22)
    -- Height
    ar[2] = BinToNumBE(d, 15, 18)
    -- Bits per channel
    ar[3] = BinToNumBE(d, 23, 24)
    -- Color mode
    n = BinToNumBE(d, 25, 26)
    if acm[n] == nil then ar[4] = '' else ar[4] = acm[n] end
    -- Number of channels
    ar[5] = BinToNumBE(d, 13, 14)
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function BinToNumBE(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = r .. string.format('%02x', string.byte(d, i))
  end
  return tonumber('0x' .. r)
end
