-- pngwdx.lua (cross-platform)
-- 2021.04.02
--[[
Getting some information from PNG files.
Supported fields: see table "fields".
]]

local fields = {
{"Width",         1},
{"Height",        1},
{"Bit depth",     1},
{"Bit per pixel", 1},
{"Color type",    8},
{"Filter",        8},
{"Interlace",     8}
}
local ac = {
[0] = "Grayscale",
[2] = "RGB",
[3] = "Indexed color (palette)",
[4] = "Grayscale with alpha channel",
[6] = "RGB with alpha channel"
}
local af = {
[0] = "None",
[1] = "Sub",
[2] = "Up",
[3] = "Average",
[4] = "Paeth"
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
  return 'EXT="PNG"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if e ~= '.png' then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000004) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000400) % 2 ~= 0 then return nil end
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    local d = h:read(8)
    local n = BinToNumBE(d, 1, 4)
    at =  BinToNumBE(d, 5, 8)
    if (n == 0x89504e47) and (at == 0x0d0a1a0a) then
      h:seek('cur', 4)
      d = h:read(4)
      if d ~= 'IHDR' then
        h:close()
        return nil
      end
      d = h:read(13)
      h:close()
      -- Width
      n = BinToNumBE(d, 1, 4)
      ar[1] = tonumber(n, 10)
      -- Height
      n = BinToNumBE(d, 5, 8)
      ar[2] = tonumber(n, 10)
      -- Bit depth
      n = string.byte(d, 9)
      ar[3] = n
      -- Color type
      at = string.byte(d, 10)
      e = ac[at]
      if e == nil then ar[5] = '' else ar[5] = e end
      -- Bit per pixel
      ar[4] = ''
      if at == 0 then
          ar[4] = n
      elseif at == 2 then
        if n == 8 then
          ar[4] = 24
        elseif n == 16 then
          ar[4] = 48
        end
      elseif at == 3 then
        ar[4] = n
      elseif at == 4 then
        if n == 8 then
          ar[4] = 16
        elseif n == 16 then
          ar[4] = 32
        end
      elseif at == 6 then
        if n == 8 then
          ar[4] = 32
        elseif n == 16 then
          ar[4] = 64
        end
      end
      -- Filter
      n = string.byte(d, 12)
      e = af[n]
      if e == nil then ar[6] = '' else ar[6] = e end
      -- Interlace
      n = string.byte(d, 13)
      if n == 0 then
        ar[7] = 'No interlace'
      elseif n == 1 then
        ar[7] = 'Adam7 interlace'
      else
        ar[7] = ''
      end
    else
      h:close()
      return nil
    end
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
