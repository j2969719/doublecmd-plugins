-- ChangeSequentialNumbers.lua (cross-platform)
--2023.12.07
--[[
Add (use "N") or subtract (use "-N") number from sequential numbers.
Supported numbering styles:
"# - filename.ext" and "filename (#).ext"

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

local aNList = {}

local function StringSplit(sdata, sdelim)
  local ares = {}
  local n1, n2, n3, c = 1, 1, 1, 1
  while true do
    n1, n2 = string.find(sdata, sdelim, n3, true)
    if n1 ~= nil then
      ares[c] = string.sub(sdata, n3, n1 - 1)
      c = c + 1
    else
      ares[c] = string.sub(sdata, n3, -1)
      break
    end
    n3 = n2 + 1
  end
  return ares
end

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

local function GetNewName(sFileName, iN)
  local sNewName
  local sPath = SysUtils.ExtractFilePath(sFileName)
  local sName = SysUtils.ExtractFileName(sFileName)
  local sBaseName = string.gsub(sName, "^(.+)%.[^%.]+$", "%1")
  local sNameExt = string.sub(sName, string.len(sBaseName) + 1, -1)

  local iN1, iN2, iTmp, sTmp
  -- "# - filename.ext"
  iN1, iN2 = string.find(sBaseName, "^%d+ %- ", 1, false)
  if iN1 ~= nil then
    sTmp = string.sub(sBaseName, iN1, iN2 - 3)
    iTmp = tonumber(sTmp) + iN
    sTmp = SetWidth(sTmp, iTmp)
    sNewName = sTmp .. string.sub(sBaseName, iN2 - 2, -1)
  else
    -- "filename (#).ext"
    iN1, iN2 = string.find(sBaseName, " %(%d+%)$", 1, false)
    if iN1 ~= nil then
      sTmp = string.sub(sBaseName, iN1 + 2, -2)
      iTmp = tonumber(sTmp) + iN
      if string.sub(sTmp, 1, 1) == "0" then
        sTmp = SetWidth(sTmp, iTmp)
      else
        sTmp = tostring(iTmp)
      end
      sNewName = string.sub(sBaseName, 1, iN1 + 1) .. sTmp .. ")"
    end
  end
  if sNewName == nil then
    return nil
  else
     sName = sTmp .. "|" .. sPath .. "|" .. sNewName .. "|" .. sNameExt .. "|" .. sFileName
     return sName
  end
end

local function GetUniqueName(sP, sN, sE)
  local sRes = sP .. sN .. sE
  local iTmp = SysUtils.FileGetAttr(sRes)
  if iTmp ~= -1 then
    local iC = 2
    while true do
      sRes = sP .. sN .. " (" .. iC .. ")" .. sE
      iTmp = SysUtils.FileGetAttr(sRes)
      if iTmp == -1 then break end
      iC = iC + 1
    end
  end
  return sRes
end

local function RenameWithNum(iN)
  local iN1, iN2, iN3
  if iN > 0 then
    iN1, iN2, iN3 = #aNList, 1, -1
  else
    iN1, iN2, iN3 = 1, #aNList, 1
  end
  local iD = 0
  for i = iN1, iN2, iN3 do
    aTmp = StringSplit(aNList[i], "|")
    sTmp = GetUniqueName(aTmp[2], aTmp[3], aTmp[4])
    bRes, sErr, iErr = os.rename(aTmp[5], sTmp)
    if bRes ~= nil then
      iD = iD + 1
    end
  end
  return iD
end

local bAct, sNum = Dialogs.InputQuery(sScrName, "Number:", false, "0")
if bAct == false then return end
local iNum = tonumber(sNum)
if iNum == 0 then return end

local h, err = io.open(params[1], "r")
if h == nil then
  Dialogs.MessageBox(params[1] .. "\nError: " .. err, sScrName, 0x0030)
  return
end
local aList = {}
local iCount = 1
local iDone
for line in h:lines() do
  aList[iCount] = line
  iCount = iCount + 1
end
h:close()

if #aList > 0 then
  iCount = 1
  for i = 1, #aList do
    sTmp = GetNewName(aList[i], iNum)
    if sTmp ~= nil then
      aNList[iCount] = sTmp
      iCount = iCount + 1
    end
  end
  if #aNList > 0 then
    table.sort(aNList)
    iDone = RenameWithNum(iNum)
    if iDone > 0 then
      DC.ExecuteCommand("cm_Refresh")
    end
  else
    Dialogs.MessageBox("There are no matching files.", sScrName, 0x0030)
  end
else
  Dialogs.MessageBox("Looks like nothing is selected.", sScrName, 0x0030)
end
