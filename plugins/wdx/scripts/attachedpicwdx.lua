-- attachedpicwdx.lua (cross-platform)
-- 2025.07.30
--[[
Getting some information about attached pictures: MP3 (ID3v2.3 & ID3v2.4) and FLAC.
(For MP3 without ID3v2, the script returns 0 and "false".)
Supported fields: see table "fields".

P.S.
GetTagSize: from Audio Tools Library
hex2bin: https://stackoverflow.com/questions/9137415/lua-writing-hexadecimal-values-as-a-binary-file
bin2dec (BinToNumS): https://stackoverflow.com/questions/37543274/how-do-i-convert-binary-to-decimal-lua
]]

local fields = {
{"Total number",     1},
{"Cover (front)",    6},
{"Cover (back)",     6},
{"Artist/performer", 6},
{"Media",            6},
{"Other",            6}
}
local ptype = {
[0x00] = 6,
[0x03] = 2,
[0x04] = 3,
[0x06] = 5,
[0x08] = 4
}
local h2b = {
["0"] = "0000", ["1"] = "0001", ["2"] = "0010", ["3"] = "0011",
["4"] = "0100", ["5"] = "0101", ["6"] = "0110", ["7"] = "0111",
["8"] = "1000", ["9"] = "1001", ["a"] = "1010", ["b"] = "1011",
["c"] = "1100", ["d"] = "1101", ["e"] = "1110", ["f"] = "1111"
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
  return '(EXT="FLAC")|(EXT="MP3")'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if (e ~= '.flac') and (e ~= '.mp3') then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    ar[1] = 0
    for i = 2, #fields do ar[i] = false end
    local c, p, s, t = 0, 0, 0, 0
    local d = h:read(4)
    if string.sub(d, 1, 4) == 'fLaC' then
      while true do
        d = h:read(4)
        t = BinToNumBEs(d, 1, 4)
        t = string.gsub(t, '[0-9a-f]', h2b)
        -- Last-metadata-block flag
        at = string.sub(t, 1, 1)
        -- Block type
        p = BinToNumS(string.sub(t, 2, 8))
        -- Size
        s = BinToNumS(string.sub(t, 9, 32))
        -- Block type == PICTURE
        if p == 6 then
          d = h:read(4)
          t = BinToNumBE(d, 1, 4)
          if ptype[t] ~= nil then ar[ptype[t]] = true end
          c = c + 1
          s = s - 4
        end
        -- Check the last-metadata-block flag
        if at == '1' then break else h:seek('cur', s) end
      end
      h:close()
      ar[1] = c
    elseif string.sub(d, 1, 3) == 'ID3' then
      d = h:read(6)
      -- ID3 size
      local id3l = GetTagSize(string.sub(d, 3, 6))
      while true do
        d = h:read(10)
        p = h:seek()
        -- Check tag name
        if string.find(d, '^[A-Z][A-Z0-9][A-Z0-9][A-Z0-9]') ~= 1 then break end
        -- Get tag data size
        s = GetTagSize(string.sub(d, 5, 8)) - 10
        if string.sub(d, 1, 4) == 'APIC' then
          s = BinToNumBE(d, 5, 8)
          -- Try to skip the text encoding and the MIME type and get the picture type
          d = h:read(30)
          for i = 2, 29 do
            if string.byte(d, i) == 0x00 then
              t = string.byte(d, i + 1)
              break
            end
          end
          if ptype[t] ~= nil then ar[ptype[t]] = true end
          c = c + 1
        end
        p = p + s
        if p >= id3l then break end
        t = h:seek('set', p)
        if t == nil then break end
      end
      h:close()
      ar[1] = c
    else
      h:close()
      -- MP3 without ID3v2:
      if BinToNumBE(d, 1, 2) ~= 0xfffb then return nil end
    end
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function GetTagSize(d)
  return d:byte(1) * 0x200000 + d:byte(2) * 0x4000 + d:byte(3) * 0x80 + d:byte(4) + 10
end

function BinToNumBE(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = r .. string.format('%02x', string.byte(d, i))
  end
  return tonumber('0x' .. r)
end

function BinToNumBEs(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = r .. string.format('%02x', string.byte(d, i))
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
