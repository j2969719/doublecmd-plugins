-- DateByEXIF.lua (cross-platform)
--2026.03.18
--[[
Sets time stamps by date from EXIF (tested with JPEG).

Requeres:
1) DC 1.1.30+;
2) the Exif plugin (Christian Ghisler's plugin):
  Win:
    https://www.ghisler.com/plugins.htm#content
  Linux and others:
    https://github.com/doublecmd/plugins/tree/master/wdx/exif

Params:
  %"0%LU
  plugin-name-in-DC-settings

Note: Check before use! This is unlikely, but the order of the Exif plugin fields may change.

Note: Script writes messages to the log window.
]]

local params = {...}

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == "@" then sScrName = string.sub(sScrName, 2, -1) end
sScrName = SysUtils.ExtractFileName(sScrName)

if #params ~= 2 then
  Dialogs.MessageBox("Check the list of parameters!", sScrName, 0x0030)
  return
end

local function GetTimeStamp(sFile, sPlug)
  local dt = {}
  local bR, sR, iD, iT
  sR = DC.GetPluginField(sFile, sPlug, 3, 0)
  if sR == nil then
    iT = DC.GetPluginField(sFile, sPlug, 6, 0)
    if iT == nil then return nil end
    sR = os.date("%Y.%m.%d", iT)
    dt.year, dt.month, dt.day = string.match(sR, "(%d%d%d%d).(%d%d).(%d%d)")
    iT = DC.GetPluginField(sFile, sPlug, 7, 0)
    sR = os.date("%H:%M:%S", iT)
    dt.hour, dt.min, dt.sec = string.match(sR, "(%d%d):(%d%d):(%d%d)")
  else
    dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec = string.match(sRes, "(%d%d%d%d).(%d%d).(%d%d).(%d%d).(%d%d).(%d%d)")
  end
  if dt.year == nil then return nil end
  for k, v in pairs(dt) do dt[k] = tonumber(v) end
  bR, iD = pcall(os.time, dt)
  if bR == false then
    return nil
  else
    return iD
  end
end

local aFiles = {}
local bRes, iDT

local h, err = io.open(params[1], "r")
if h == nil then
  Dialogs.MessageBox("Error: " .. err, sScrName, 0x0030)
  return
end
for l in h:lines() do
  table.insert(aFiles, l)
end
h:close()
if #aFiles == 0 then
  Dialogs.MessageBox("Nothing selected?", sScrName, 0x0030)
  return
end

for i = 1, #aFiles do
  iDT = GetTimeStamp(aFiles[i], params[2])
  if iDT == nil then
    DC.LogWrite(aFiles[i] .. ": Couldn't get the date.", 2, false, false)
  else
    bRes = SysUtils.FileSetTime(aFiles[i], iDT, iDT, iDT)
    if bRes == false then
      DC.LogWrite(aFiles[i] .. ": Couldn't set time stamps.", 2, false, false)
    end
  end
end
DC.LogWrite(sScrName .. ": Done.", 1, false, false)
