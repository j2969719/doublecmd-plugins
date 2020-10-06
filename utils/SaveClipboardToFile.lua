-- SaveClipboardToFile.lua (cross-platform)
-- 2020.10.05
--[[
Save clipboard contents to text file

How to use: add button with
  internal command: cm_ExecuteScript
  params:
    path-to-script-SaveClipboardToFile.lua
    %"0%Ds

File extension is "txt" by default.
]]

local params = {...}

local sn = debug.getinfo(1).source
if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
sn = SysUtils.ExtractFileName(sn)

if #params ~= 1 then
  Dialogs.MessageBox('Check the number of parameters! Requires only one.', sn, 0x0030)
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

local h = io.open(fn, 'w')
if h == nil then
  Dialogs.MessageBox('Failed to create "' ..  nn .. '.txt".', sn, 0x0030)
  return
end
h:write(fc)
h:close()
