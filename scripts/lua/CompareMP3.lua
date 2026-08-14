-- CompareMP3.lua
-- 2026.08.14
--[[
Compare only MP3 streams, without tags (ID3v1/ID3v2/APEv2).

Params:

- in one panel:
    %"0%p1
    %"0%p2
- in different panels:
    %"0%ps0
    %"0%pt0
- with the same name in another panel:
    %"0%ps0
    %"0%/1%Dt%fs0

P.S.
GetTagSize: from Audio Tools Library
]]

local params = {...}

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == "@" then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

if #params ~= 2 then
  Dialogs.MessageBox("Check the list of parameters!", sScrName, 0x0030)
  return
end

local function GetTagSize(d)
  return d:byte(1) * 0x200000 + d:byte(2) * 0x4000 + d:byte(3) * 0x80 + d:byte(4) + 10
end

local function BinToNumLE(d, n1, n2)
  local r = ""
  for i = n1, n2 do
    r = string.format("%02x", string.byte(d, i)) .. r
  end
  return tonumber("0x" .. r)
end

local function GetData(sFN)
  local iS = SysUtils.GetFileProperty(sFN, 0)
  local h = io.open(sFN, "rb")
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
  local r = h:read(iPE - iPS)
  h:close()
  return r
end

local f1 = GetData(params[1])
local f2 = GetData(params[2])
if (f1 == nil) or (f2 == nil) then
  Dialogs.MessageBox("Unknown error.", sScrName, 0x0030)
  return
end

if string.len(f1) ~= string.len(f2) then
  Dialogs.MessageBox("These two files are not identical:\n\n" .. params[1] .. "\n\n" .. params[2], sScrName, 0x0040)
  return
end

local b = true
local d1, d2
local i = 1
while true do
  d1 = string.sub(f1, i, i + 4095)
  d2 = string.sub(f2, i, i + 4095)
  if string.len(d1) == 0 then break end
  if d1 ~= d2 then
    b = false
    break
  end
  i = i + 4096
end

if b == true then
  Dialogs.MessageBox("These two files are identical:\n\n" .. params[1] .. "\n\n" .. params[2], sScrName, 0x0040)
else
  Dialogs.MessageBox("These two files are not identical:\n\n" .. params[1] .. "\n\n" .. params[2], sScrName, 0x0040)
end
