-- webpwdx.lua (cross-platform)
-- 2021.09.12
--[[
Getting some information from WebP files.
Supported fields: see table "fields".

Notes about animated images:
1. "Compression" will return "unknown", bacause each frame can be compressed regardless of others.
2. "Transparency" can be return "false" (if the "Alpha" flag in "VP8X" chunk is unset), but some
   separate frames may contain an alpha channel.

P.S.
hex2bin: https://stackoverflow.com/questions/9137415/lua-writing-hexadecimal-values-as-a-binary-file
bin2dec (BinToNumS): https://stackoverflow.com/questions/37543274/how-do-i-convert-binary-to-decimal-lua
]]

local fields = {
{"Width",           "", 1},
{"Height",          "", 1},
{"Compression",     "lossy|lossless|unknown", 7},
{"Extended Format", "", 6},
{"Animation",       "", 6},
{"Transparency",    "", 6},
{"ICC profile",     "", 6},
{"EXIF",            "", 6},
{"XMP",             "", 6}
}
local ar = {}
local h2b = {
["0"] = "0000", ["1"] = "0001", ["2"] = "0010", ["3"] = "0011",
["4"] = "0100", ["5"] = "0101", ["6"] = "0110", ["7"] = "0111",
["8"] = "1000", ["9"] = "1001", ["a"] = "1010", ["b"] = "1011",
["c"] = "1100", ["d"] = "1101", ["e"] = "1110", ["f"] = "1111"
}
local filename = ""

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
  return 'EXT="WEBP"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if e ~= '.webp' then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    local d = h:read(12)
    if (string.sub(d, 1, 4) == 'RIFF') and (string.sub(d, 9, 12) == 'WEBP') then
      for i = 1, #fields do ar[i] = '' end
      -- File size
      local fs = BinToNumLE(d, 5, 8) + 8
      local t
      d = h:read(4)
      -- Extended Format
      if d == 'VP8X' then
        ar[4] = true
        h:seek('cur', 4)
        d = h:read(1)
        t = string.format('%02x', string.byte(d))
        d = string.gsub(t, '[0-9a-f]', h2b)
        -- ICC profile
        if string.sub(d, 3, 3) == '1' then ar[7] = true else ar[7] = false end
        -- Alpha
        if string.sub(d, 4, 4) == '1' then ar[6] = true else ar[6] = false end
        -- EXIF
        if string.sub(d, 5, 5) == '1' then ar[8] = true else ar[8] = false end
        -- XMP
        if string.sub(d, 6, 6) == '1' then ar[9] = true else ar[9] = false end
        -- Animation
        if string.sub(d, 7, 7) == '1' then
          ar[5] = true
          ar[3] = 'unknown'
        else
          ar[5] = false
        end
        d = h:read(9)
        ar[1] = BinToNumLE(d, 4, 6) + 1
        ar[2] = BinToNumLE(d, 7, 9) + 1
        if ar[5] == false then
          while true do
            d = h:read(4)
            if d == 'VP8 ' then
              ar[3] = 'lossy'
              break
            elseif d == 'VP8L' then
              ar[3] = 'lossless'
              break
            else
              d = h:read(4)
              t = BinToNumLE(d, 1, 4)
              -- Fix odd number
              if math.mod(t, 2) ~= 0 then t = t + 1 end
              t = h:seek('cur', t)
              if (t == nil) or (t >= fs) then break end
            end
          end
          if ar[3] == '' then
            h:close()
            return nil
          end
        end
      -- Simple format, lossy
      elseif d == 'VP8 ' then
        ar[3] = 'lossy'
        h:seek('cur', 7)
        -- Check sync code
        d = h:read(3)
        if (string.byte(d, 1) == 0x9d) and (string.byte(d, 2) == 0x01) and (string.byte(d, 3) == 0x2a) then
          d = h:read(4)
          t = BinToNumLEs(d, 1, 4)
          t = string.gsub(t, '[0-9a-f]', h2b)
          ar[1] = BinToNumS(string.sub(t, 19, 32))
          ar[2] = BinToNumS(string.sub(t, 3, 16))
        else
          h:close()
          return nil
        end
      -- Simple format, lossless
      elseif d == 'VP8L' then
        ar[3] = 'lossless'
        h:seek('cur', 4)
        -- Check signature
        d = h:read(1)
        if string.byte(d) == 0x2f then
          d = h:read(4)
          t = BinToNumLEs(d, 1, 4)
          t = string.gsub(t, '[0-9a-f]', h2b)
          ar[1] = BinToNumS(string.sub(t, 19, 32)) + 1
          ar[2] = BinToNumS(string.sub(t, 5, 18)) + 1
        else
          h:close()
          return nil
        end
      else
        h:close()
        return nil
      end
      h:close()
      if ar[4] ~= true then ar[4] = false end
    end
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function BinToNumLE(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = string.format('%02x', string.byte(d, i)) .. r
  end
  return tonumber('0x' .. r)
end

function BinToNumLEs(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = string.format('%02x', string.byte(d, i)) .. r
  end
  return r
end

function BinToNumS(bin)
  bin = string.reverse(bin)
  local sum = 0
  for i = 1, string.len(bin) do
    num = string.sub(bin, i, i) == '1' and 1 or 0
    sum = sum + num * math.pow(2, i - 1)
  end
  return sum
end
