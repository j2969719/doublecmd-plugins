-- M3U8Check.lua (cross-platform)
-- 2026.03.20
--[[
The script checks that all files are available and the list does not contain duplicates.

Params: %"0%p0
]]

local params = {...}

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == "@" then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

if #params == 0 then
  Dialogs.MessageBox("Check the list of parameters.", sScrName, 0x0030)
  return
end

local aM3U = {}
local aDup = {}

local n1, n2, sT
if SysUtils.PathDelim == "/" then
  n1, n2, sT = 1, 1, "/"
else
  n1, n2, sT = 2, 3, ":\\"
end

local iC, iC2 = 1, 1
local sFP = SysUtils.ExtractFilePath(params[1])
local h = io.open(params[1], "r")
if h == nil then
  Dialogs.MessageBox('Failed to open\n"' .. params[1] '"', sScrName, 0x0030)
  return
end
for l in h:lines() do
  if string.sub(l, 1, 1) ~= "#" then
    if string.sub(l, n1, n2) == sT then
      aM3U[iC] = string.gsub(l, "[\r\n]+$", "")
      iC = iC + 1
    end
  end
end
h:close()
if #aM3U == 0 then
  Dialogs.MessageBox("List is empty or does not contain any files.", sScrName, 0x0040)
  return
end

iC = 0
for i = 1, #aM3U do
  -- Skip network protocols (ftp, http, https, mms, rtmp, rtp, rtsp, udp)
  if string.match(aM3U[i], "^[a-z][a-z][a-z]+://.*") == nil then
    -- Check full (absolute) filename
    if SysUtils.FileGetAttr(aM3U[i]) == -1 then
      -- Check relative filename
      if SysUtils.FileGetAttr(sFP .. aM3U[i]) == -1 then
        DC.LogWrite(sScrName .. ": Not found: " .. aM3U[i], 2, true, false)
        iC = iC + 1
      end
    end
  end
end
if iC == 0 then
  DC.LogWrite(sScrName .. ": All files have been found.", 1, true, false)
end

iC = 1
table.sort(aM3U)
while true do
  if aM3U[iC] == aM3U[iC + 1] then
    aDup[iC2] = aM3U[iC]
    iC2 = iC2 + 1
    for i = iC + 1, #aM3U do
      if aM3U[i] ~= aM3U[i + 1] then
        iC = i + 1
        break
      end
    end
  else
    iC = iC + 1
  end
  if iC >= #aM3U then break end
end

if #aDup == 0 then
  DC.LogWrite(sScrName .. ": No duplicates found.", 1, true, false)
else
  for i = 1, #aDup do
    DC.LogWrite(sScrName .. ": Duplicated: " .. aDup[i], 2, true, false)
  end
end
Dialogs.MessageBox("Done:\n" .. params[1], sScrName, 0x0040)
