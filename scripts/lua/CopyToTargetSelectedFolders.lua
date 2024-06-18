-- CopyToTargetSelectedFolders.lua (cross-platform)
-- 2024.06.04
--[[
Copy selected files to all selected folders in the target directory.

The script uses the navigation commands and "cm_Copy" with "confirmation=" & "queueid=1"
(see details here https://doublecmd.github.io/doc/en/cmds.html#cm_Copy).

How to use: add button (https://doublecmd.github.io/doc/en/toolbar.html) with
internal command: cm_ExecuteScript
params:
  path-to-script.lua
  %"0%LUt
  %"0%Dt
  confirmation

"confirmation" is optional parameter: copy operation will begin with a confirmation window
(https://doublecmd.github.io/doc/en/copymove.html#confirmation) for each folder.
]]

local params = {...}

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == "@" then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

if #params < 2 then
  Dialogs.MessageBox("Check the list of parameters!", sScrName, 0x0030)
  return
end

local aDirs  = {}
local sConfirmation = "0"
if (#params == 3) and (params[3] == "confirmation") then
  sConfirmation = "1"
end

local function GetTargetDirs(p)
  local h, err = io.open(p, "r")
  if h == nil then
    Dialogs.MessageBox("Error: " .. err, sScrName, 0x0030)
    return nil
  end
  local i
  for l in h:lines() do
    i = SysUtils.FileGetAttr(l)
    if math.floor(i / 0x00000010) % 2 ~= 0 then
      table.insert(aDirs, l)
    end
  end
  h:close()
end

local function QuoteIt(s)
  if string.find(s, " ", 1, true) == nil then return s end
  if SysUtils.PathDelim == "\\" then
    return '"' .. s .. '"'
  else
    return string.gsub(s, " ", "\\ ")
  end
end

local function CDAndCopy(sDir)
  DC.ExecuteCommand("cm_FocusSwap")
  DC.ExecuteCommand("cm_ChangeDir", QuoteIt(sDir))
  DC.ExecuteCommand("cm_FocusSwap")
  DC.ExecuteCommand("cm_SaveSelection")
  DC.ExecuteCommand("cm_Copy", "confirmation=" .. sConfirmation, "queueid=1")
  DC.ExecuteCommand("cm_RestoreSelection")
end

GetTargetDirs(params[1])
if #aDirs == 0 then
  Dialogs.MessageBox("Nothing selected?", sScrName, 0x0030)
  return
end

local sTmp
if #aDirs > 3 then
  sTmp = table.concat(aDirs, "\n", 1, 3) .. "\n..."
else
  sTmp = table.concat(aDirs, "\n")
end
local sMsg = "Copy to all selected folders (" .. #aDirs .. ") in the target directory?\n\n" .. sTmp
local iRet = Dialogs.MessageBox(sMsg, sScrName, 0x0001 + 0x0020)
if iRet == 0x0001 then
  for i = 1, #aDirs do
    CDAndCopy(aDirs[i])
  end
  DC.ExecuteCommand("cm_FocusSwap")
  DC.ExecuteCommand("cm_ChangeDir", QuoteIt(params[2]))
  DC.ExecuteCommand("cm_FocusSwap")
end
