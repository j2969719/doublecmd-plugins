-- SaveClipboardToFile.lua (cross-platform)
-- 2020.11.02
--[[
Save clipboard contents to text file
File extension is "txt" by default.

How to use: add button with
  internal command: cm_ExecuteScript
  params:
    path-to-script-SaveClipboardToFile.lua
    %"0%Ds

Also supports additional parameters (optional)
  --view
  --edit
if you want to open this new file in viewer or editor immediately.
]]

local params = {...}

local sn = debug.getinfo(1).source
if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
sn = SysUtils.ExtractFileName(sn)

if (#params ~= 1) and (#params ~= 2) then
  Dialogs.MessageBox('Check the number of parameters! Requires only one or two.', sn, 0x0030)
  return
end

local fc = Clipbrd.GetAsText()
if (fc == nil) or (fc == '') then
  Dialogs.MessageBox('Clipboard is empty.', sn, 0x0040)
  return
end

local fn, ba, bp, nn
while true do
  ba, nn = Dialogs.InputQuery(sn, 'Enter filename:', false, 'new')
  if ba == false then return end
  fn = params[1] .. SysUtils.PathDelim .. nn .. '.txt'
  if SysUtils.FileExists(fn) == false then break end
  bp = Dialogs.MessageBox('File "' .. nn .. '.txt" already exists!\n\nOverwrite?', sn, 0x0003 + 0x0030 + 0x0100)
  if bp == 0x0006 then
    break
  else
    if bp ~= 0x0007 then return end
  end
end

local mode = 'wb'
if SysUtils.FileExists(fn) == true then mode = 'w+b' end
local h = io.open(fn, mode)
if h == nil then
  Dialogs.MessageBox('Failed to create "' ..  nn .. '.txt".', sn, 0x0030)
  return
end
h:write(fc)
h:close()

if #params == 2 then
  local cm
  if params[2] == '--view' then
    cm = 'cm_View'
  elseif params[2] == '--edit' then
    cm = 'cm_Edit'
  else
    Dialogs.MessageBox('Unknown parameter "' .. params[2] .. '"!', sn, 0x0030)
    return
  end
  DC.ExecuteCommand('cm_Refresh')
  DC.ExecuteCommand('cm_QuickSearch', 'search=on', 'direction=first', 'matchbeginning=on', 'matchending=off', 'casesensitive=on', 'files=on', 'directories=off', 'text=' .. nn .. '.txt')
  DC.ExecuteCommand('cm_QuickSearch', 'search=off')
  DC.ExecuteCommand(cm)
end
