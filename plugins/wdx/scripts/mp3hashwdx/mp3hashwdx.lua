-- mp3hashwdx.lua (cross-platform)
-- 2026.10.26
--[[
Getting hash of MP3 stream as is, i.e. without tags (ID3v1/ID3v2/APEv2).
(Logical continuation of
https://github.com/j2969719/doublecmd-plugins/blob/master/scripts/lua/CompareMP3.lua)

Requires sha2.lua https://github.com/Egor-Skriptunoff/pure_lua_SHA

P.S.
GetTagSize: from Audio Tools Library
]]

local r, sha = pcall(require, "sha2")

local filename = ""
local data

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "BLAKE2s", "", 8
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="MP3"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if r == false then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if e ~= ".mp3" then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    local h = io.open(FileName, "rb")
    if GetData(FileName) == nil then return nil end
    filename = FileName
  end
  if FieldIndex == 0 then
    return sha.blake2s(data)
  end
  return nil
end

function GetData(sF)
  local iS = SysUtils.GetFileProperty(sF, 0)
  local h = io.open(sF, "rb")
  if h == nil then return nil end
  local iID3S, iPS, iPE, iP
  local iPExA, iPExI = 0, 0
  -- Skip ID3v2
  local d = h:read(10)
  if string.sub(d, 1, 3) == "ID3" then
    -- ID3v2 size
    iID3S = GetTagSize(string.sub(d, 7, 10))
    iPS = iID3S
  else
    iPS = 0
  end
  -- Skip APEv2
  h:seek("set", iS - 32)
  d = h:read(16)
  if string.sub(d, 1, 8) == "APETAGEX" then
    iPExA = BinToNumLE(d, 13, 16) + 32
  else
    -- APEv2 + ID3v1?
    h:seek("set", iS - 160)
    d = h:read(16)
    if string.sub(d, 1, 8) == "APETAGEX" then
      iPExA = BinToNumLE(d, 13, 16) + 32
    end
    -- Skip ID3v1
    h:seek("set", iS - 128)
    d = h:read(3)
    if d == "TAG" then
      iPExI = 128
    end
  end
  iPE = iS - (iPExA + iPExI)
  h:seek("set", iPS)
  data = h:read(iPE - iPS)
  h:close()
  return 1
end

function GetTagSize(d)
  return d:byte(1) * 0x200000 + d:byte(2) * 0x4000 + d:byte(3) * 0x80 + d:byte(4) + 10
end

function BinToNumLE(d, n1, n2)
  local r = ""
  for i = n1, n2 do
    r = string.format("%02x", string.byte(d, i)) .. r
  end
  return tonumber("0x" .. r)
end
