-- LoadSaveFavFiles.lua (cross-platform)
-- 2023.08.09
--[[
List of favorite files (with https://doublecmd.github.io/doc/en/cmds.html#cm_LoadList):
1) if there are selected files: add to the list, exclude from the list or load files from the list;
2) if nothing is selected: load files from the list.

The list will be saved in "favoritefiles.txt" in the DC configuration files directory
(see https://doublecmd.github.io/doc/en/configuration.html#ConfigDC).
If you want to change the filename: see the "sListFile" value below.

How to use:
internal command: cm_ExecuteScript
params:
  path-to-script
  %"0%LU
  newtab

"newtab" is optional parameter: the script will load the list of files in a new tab.
]]

local params = {...}

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == '@' then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

if (#params < 1) or (#params > 2) then
  Dialogs.MessageBox("Check the number of parameters!", sScrName, 0x0030)
  return
end

-- CheckSize() and GetList() are helper functions

local function CheckSize(sF)
  local iM -- Min. file size
  if SysUtils.PathDelim == "\\" then iM = 6 else iM = 3 end
  local h, aD = SysUtils.FindFirst(sF)
  if h ~= nil then
    SysUtils.FindClose(h)
    if aD.Size >= iM then return true end
  end
  return false
end

local function GetList(sF)
  local aR = {}
  local iC = 1
  local h, err = io.open(sF, "r")
  if h ~= nil then
    for l in h:lines() do
      aR[iC] = string.gsub(l, "%s+$", "")
      iC = iC + 1
    end
    h:close()
  end
  return aR
end

-- Get the full filename of the list
local sTmp = os.getenv("COMMANDER_INI")
if sTmp == nil then
  Dialogs.MessageBox("Could not find the directory with the DC configuration files.", sScrName, 0x0030)
  return
end
local sListFile = SysUtils.ExtractFilePath(sTmp) .. "favoritefiles.txt"

-- Check if files are selected...
sTmp = os.tmpname()
DC.ExecuteCommand("cm_SaveSelectionToFile", sTmp)
local bS = CheckSize(sTmp)
os.remove(sTmp)
local bL = CheckSize(sListFile)
if (bS == false) and (bL == false) then
  -- ... no and the list of favorite files is empty
  Dialogs.MessageBox("The list of favorite files is empty and nothing is selected.", sScrName, 0x0030)
  return
elseif (bS == false) and (bL == true) then
  -- ... no and the list of favorite files is not empty
  if params[2] == "newtab" then
    DC.ExecuteCommand("cm_NewTab")
  end
  DC.ExecuteCommand("cm_LoadList", "filename=" .. sListFile)
  return
end

-- Create/update/load the list of favorite files
local aActions = {
"Add selected files to the list",
"Exclude selected files from the list",
"Load files from the list"
}
local sRet = Dialogs.InputListBox(sScrName, "Choose an action:", aActions, aActions[1])
if sRet == nil then return end
if sRet == "Load files from the list" then
  if params[2] == "newtab" then
    DC.ExecuteCommand("cm_NewTab")
  end
  DC.ExecuteCommand("cm_LoadList", "filename=" .. sListFile)
  return
end

local aList = GetList(sListFile)
local aListN = GetList(params[1])
local aRes = {}
local iCount
if sRet == "Add selected files to the list" then
  if #aList == 0 then
    aRes = aListN
    table.sort(aRes)
  else
    iCount = #aList + 1
    for i = 1, #aListN do
      aList[iCount] = aListN[i]
      iCount = iCount + 1
    end
    -- Remove duplicates
    table.sort(aList)
    aRes[1] = aList[1]
    iCount = 2
    for i = 2, #aList do
      if aList[i] ~= aList[i - 1] then
        aRes[iCount] = aList[i]
        iCount = iCount + 1
      end
    end
  end
elseif sRet == "Exclude selected files from the list" then
  if #aList == 0 then
    Dialogs.MessageBox("The list of favorite files is empty.", sScrName, 0x0030)
    return
  else
    iCount = 1
    sTmp = "|" .. table.concat(aListN, "|") .. "|"
    for i = 1, #aList do
      if string.find(sTmp, "|" .. aList[i] .. "|", 1, true) == nil then
        aRes[iCount] = aList[i]
        iCount = iCount + 1
      end
    end
  end
else
  Dialogs.MessageBox("Unknown error.", sScrName, 0x0030)
  return
end

-- Save
if #aRes == 0 then
  os.remove(sListFile)
  return
end
local h, err = io.open(sListFile, "w+")
if h == nil then
  Dialogs.MessageBox(sListFile .. "\nError: " .. err, sScrName, 0x0030)
  return
end
h:write(table.concat(aRes, "\n") .. "\n")
h:close()
Dialogs.MessageBox("The list of favorite files has been updated/created.", sScrName, 0x0040)
