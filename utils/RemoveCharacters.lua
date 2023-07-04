-- RemoveCharacters.lua (cross-platform)
-- 2023.07.02
--[[
Remove N characters from the beginning (use "N") or end (use "-N") of the selected filenames.
Spaces/dots at the beginning of the name or spaces at the end will be removed.

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

local function RenameWithCut(sFileName, iN, bB)
  local sNewName
  local sPath = SysUtils.ExtractFilePath(sFileName)
  local sName = SysUtils.ExtractFileName(sFileName)
  local sBaseName = string.gsub(sName, "^(.+)%.[^%.]+$", "%1")
  local sNameExt = string.sub(sName, string.len(sBaseName) + 1, -1)

  local iLength = LazUtf8.Length(sBaseName)
  if iN >= iLength then
    Dialogs.MessageBox('The number ' .. iN .. ' is greater than the length of the name\n"' .. sFileName .. '"', sScrName, 0x0030)
    return 0
  end

  if bB == true then
    sNewName = LazUtf8.Copy(sBaseName, iN + 1, iLength - iN)
    sNewName = string.gsub(sNewName, "^[%. ]+", "")
  elseif bB == false then
    sNewName = LazUtf8.Copy(sBaseName, 1, iLength - iN)
    sNewName = string.gsub(sNewName, "%s+$", "")
  end
  if sNewName == "" then
    Dialogs.MessageBox('The number ' .. iN .. ' is incorrect value for this file\n"' .. sFileName .. '"', sScrName, 0x0030)
    return 0
  end

  local bResult, sError, iError = os.rename(sFileName, sPath .. sNewName)
  if bResult == nil then
    Dialogs.MessageBox("Error " .. iError .. ": " .. sError, sScrName, 0x0030)
    return 0
  else
    return 1
  end
end

local bBegin = true

local bAct, sNum = Dialogs.InputQuery(sScrName, "Remove N characters from the beginning (N) or end (-N) of the name:", false, "0")
if bAct == false then return end
if string.sub(sNum, 1, 1) == "-" then
  bBegin = false
  sNum = string.sub(sNum, 2, -1)
end
local iNum = tonumber(sNum)
if iNum == 0 then return end

local h, err = io.open(params[1], "r")
if h == nil then
  Dialogs.MessageBox(params[1] .. "\nError: " .. err, sScrName, 0x0030)
  return
end
local aList = {}
local iCount = 1
for line in h:lines() do
  aList[iCount] = line
  iCount = iCount + 1
end
h:close()

if #aList > 0 then
  iCount = 0
  for i = 1, #aList do
    iCount = iCount + RenameWithCut(aList[i], iNum, bBegin)
  end
  if iCount > 0 then
    DC.ExecuteCommand("cm_Refresh")
  end
else
  Dialogs.MessageBox("Looks like nothing is selected.", sScrName, 0x0030)
end
