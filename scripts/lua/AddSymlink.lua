-- AddSymlink.lua (cross-platform)
-- 2025.05.07
--[[
Create symbolic links (in an inactive panel) for selected files or
the file under the cursor.

Parameters:
  %"0%LU
  %"0%Dt
  relative

"relative" is an optional parameter:
 symbolic link(s) with a relative path (if possible).
]]

local params = {...}

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == "@" then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

if (#params ~= 2) and (#params ~= 3) then
  Dialogs.MessageBox("Check the list of parameters!", sScrName, 0x0030)
  return
end

local aFiles = {}
local bRes, sFile, sLink

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
  sLink = params[2] .. SysUtils.PathDelim .. SysUtils.ExtractFileName(aFiles[i])
  if SysUtils.FileGetAttr(sLink) ~= -1 then
    DC.LogWrite(sScrName .. ": already exists: " .. sLink, 2, false, false)
  else
    if params[3] == "relative" then
      sFile = SysUtils.GetRelativePath(aFiles[i], params[2])
    else
      sFile = aFiles[i]
    end
    if sFile == "" then
      DC.LogWrite(sScrName .. ": unknown error 2: " .. sLink, 2, false, false)
    else
      bRes = SysUtils.CreateSymbolicLink(sFile, sLink)
      if bRes == true then
        DC.LogWrite(sScrName .. ": created: " .. sLink, 1, false, false)
      else
        DC.LogWrite(sScrName .. ": unknown error: " .. sLink, 2, false, false)
      end
    end
  end
end
