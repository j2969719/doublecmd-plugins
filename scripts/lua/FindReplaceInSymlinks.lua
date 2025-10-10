-- FindReplaceInSymlinks.lua (cross-platform)
-- 2025.10.09
--[[
Find and replace text within the targets of selected symbolic links
(case sensitive!). Supports environment variables (but only "\$[A-Z]+" yet).
Use "find|replace" (without quotes).
Error messages will be written to the log window.

Parameters:
  %"0%LU
]]

local params = {...}

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == "@" then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

if #params ~= 1 then
  Dialogs.MessageBox("Check the list of parameters!", sScrName, 0x0030)
  return
end

local aFiles = {}
local sSearch, sReplace

local function StringReplace(sOrig, sFind, sRep)
  local sRes = ""
  local n1, n2, n3 = 1, 1, 1
  while true do
    n1, n2 = string.find(sOrig, sFind, n3, true)
    if n1 ~= nil then
      sRes = sRes .. string.sub(sOrig, n3, n1 - 1) .. sRep
    else
      sRes = sRes .. string.sub(sOrig, n3, -1)
      break
    end
    n3 = n2 + 1
  end
  return sRes
end

local function ChangeSymlink(sFile)
  local sTarget = SysUtils.ReadSymbolicLink(sFile, false)
  if sTarget == "" then
    DC.LogWrite(sScrName .. ' Error: "' .. sFile .. '" - Failed to read target.', 2, false, false)
    return
  end
  local sNewTarget = StringReplace(sTarget, sSearch, sReplace)
  if sNewTarget ~= sTarget then
    local sTmp = sFile .. "bak1"
    local bRes, sErr, iErr = os.rename(sFile, sTmp)
    if bRes == nil then
      DC.LogWrite(sScrName .. ' Error: "' .. sFile .. '" - ' .. sErr, 2, false, false)
      return
    end
    bRes = SysUtils.CreateSymbolicLink(sNewTarget, sFile)
    if bRes == false then
      os.rename(sTmp, sFile)
      DC.LogWrite(sScrName .. ' Error: "' .. sFile .. '" - SysUtils.CreateSymbolicLink()', 2, false, false)
    else
      os.remove(sTmp)
    end
  end
end

-- Get strings
local bRes, sRes = Dialogs.InputQuery(sScrName, "Find and replace:", false, "find|replace")
if bRes == false then return end
-- Expand environment variables
-- (maybe "%$([A-Z][A-Z_]+)" or "%$([A-Z][A-Z0-9_]+)"?)
sRes = string.gsub(sRes, "%$([A-Z]+)", os.getenv)
-- Split
local n = string.find(sRes, "|", 1, true)
if n == nil then
  Dialogs.MessageBox("Separator is not found.", sScrName, 0x0030)
  return
else
  sSearch = string.sub(sRes, 1, n - 1)
  sReplace = string.sub(sRes, n + 1, -1)
end

-- Read list
local h, err = io.open(params[1], "r")
if h == nil then
  Dialogs.MessageBox("Error: " .. err, sScrName, 0x0030)
  return
end
for l in h:lines() do
  n = SysUtils.FileGetAttr(l)
  if n < 0 then
    DC.LogWrite(sScrName .. ' Error: "' .. l .. '" - SysUtils.FileGetAttr()', 2, false, false)
  end
  if math.floor(n / 0x00000400) % 2 == 0 then
    DC.LogWrite(sScrName .. ': "' .. l .. '" - Not a symbolic link.', 2, false, false)
  else
    table.insert(aFiles, l)
  end
end
h:close()
if #aFiles == 0 then
  Dialogs.MessageBox("Nothing selected?", sScrName, 0x0030)
  return
end

-- Process
for i = 1, #aFiles do
  ChangeSymlink(aFiles[i])
end

Dialogs.MessageBox("Done!", sScrName, 0x0040)
