-- ChangeSequentialNumbers.lua (cross-platform)
--2023.12.09
--[[
Add (use "N") or subtract (use "-N") number from sequential numbers.
Supported numbering styles:
N - filename.ext and N - foldername,
filename (N).ext and foldername (N).

Details: https://doublecmd.h1n.ru/viewtopic.php?t=7793

How to use:
command: cm_ExecuteScript
params:
  path-to-script
  %"0%LU
]]

local params = {...}

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == '@' then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

if #params ~= 1 then
  Dialogs.MessageBox("Check the number of parameters!", sScrName, 0x0030)
  return
end

local aTmp = {}
local iNum

local function SetWidth(sN, iN)
  local iL = string.len(sN)
  local sT = tostring(iN)
  while true do
    if string.len(sT) < iL then
      sT = "0" .. sT
    else
      break
    end
  end
  return sT
end

local function GetNewName(sFileName)
  local sBaseName, sExt, sNewBaseName
  local sPath = SysUtils.ExtractFilePath(sFileName)
  local sName = SysUtils.ExtractFileName(sFileName)
  local iAttr = SysUtils.FileGetAttr(sFileName)
  if math.floor(iAttr / 0x00000010) % 2 ~= 0 then
    sBaseName = sName
    sExt = ""
  else
    sBaseName = string.gsub(sName, "^(.+)%.[^%.]+$", "%1")
    sExt = string.sub(sName, string.len(sBaseName) + 1, -1)
  end
  local iN1, iN2, iTmp, sTmp
  -- "# - filename.ext"
  iN1, iN2 = string.find(sBaseName, "^%d+ %- ", 1, false)
  if iN1 ~= nil then
    sTmp = string.sub(sBaseName, iN1, iN2 - 3)
    iTmp = tonumber(sTmp) + iNum
    sTmp = SetWidth(sTmp, iTmp)
    sNewBaseName = sTmp .. string.sub(sBaseName, iN2 - 2, -1)
  else
    -- "filename (#).ext"
    iN1, iN2 = string.find(sBaseName, " %(%d+%)$", 1, false)
    if iN1 ~= nil then
      sTmp = string.sub(sBaseName, iN1 + 2, -2)
      iTmp = tonumber(sTmp) + iNum
      if string.sub(sTmp, 1, 1) == "0" then
        sTmp = SetWidth(sTmp, iTmp)
      else
        sTmp = tostring(iTmp)
      end
      sNewBaseName = string.sub(sBaseName, 1, iN1 + 1) .. sTmp .. ")"
    end
  end
  if sNewBaseName == nil then
    return false
  else
    aTmp[1] = sPath
    aTmp[2] = sNewBaseName
    aTmp[3] = sExt
    return true
  end
end

local function GetUniqueName(sP, sN, sE)
  local sRes = sP .. sN .. sE
  local iAt = SysUtils.FileGetAttr(sRes)
  if iAt ~= -1 then
    local iC = 2
    while true do
      sRes = sP .. sN .. " (" .. iC .. ")" .. sE
      iAt = SysUtils.FileGetAttr(sRes)
      if iAt == -1 then break end
      iC = iC + 1
    end
  end
  return sRes
end

local bAct, sNum = Dialogs.InputQuery(sScrName, "Number:", false, "0")
if bAct == false then return end
iNum = tonumber(sNum)
if (iNum == nil) or (iNum == 0) then
  Dialogs.MessageBox("Incorrect value: " .. sNum, sScrName, 0x0030)
  return
end

local h, err = io.open(params[1], "r")
if h == nil then
  Dialogs.MessageBox(params[1] .. "\nError: " .. err, sScrName, 0x0030)
  return
end
local aList = {}
for line in h:lines() do
  table.insert(aList, line)
end
h:close()

if #aList == 0 then
  Dialogs.MessageBox("Looks like nothing is selected.", sScrName, 0x0030)
  return
end

local iN1, iN2, iN3, sTmp
local bRes, sErr, iErr
local iDone = 0

if iNum > 0 then
  iN1, iN2, iN3 = #aList, 1, -1
else
  iN1, iN2, iN3 = 1, #aList, 1
end
for i = iN1, iN2, iN3 do
  if GetNewName(aList[i]) == true then
    sTmp = GetUniqueName(aTmp[1], aTmp[2], aTmp[3])
    bRes, sErr, iErr = os.rename(aList[i], sTmp)
    if bRes ~= nil then
      iDone = iDone + 1
    end
  end
end

if iDone > 0 then
  DC.ExecuteCommand("cm_Refresh")
end
