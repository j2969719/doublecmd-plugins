-- Create new file
-- 2018.11.17
--
-- Params:
--   %"0%D
--   %dir-with-templates%
--
-- Button example:
--[[
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{78AE07D0-42A0-4238-92BB-BE802AFB6F48}</ID>
    <Icon>cm_executescript</Icon>
    <Hint>Create new file</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/utils/CreateNewFile.lua</Param>
    <Param>%"0%D</Param>
    <Param>%COMMANDER_PATH%/utils/newfiles</Param>
  </Command>
</doublecmd>
]]

local params = {...}
local items = {}
local ba, ct, fd, fl, hd, rt, sn, st

if (#params ~= 2) then
  Dialogs.MessageBox('Check the number of parameters!\nRequires only two.', 'Create new file', 0x0030);
  return;
end

hd, fd = SysUtils.FindFirst(params[2] .. SysUtils.PathDelim .. '*');
if (hd ~= nil) then
  repeat
    if (fd.Name ~= '.') and (fd.Name ~= '..') then table.insert(items, fd.Name) end;
    rt, fd = SysUtils.FindNext(hd);
  until rt == nil
  SysUtils.FindClose(hd);
end
if (#items >= 1) then
  st = Dialogs.InputListBox('Create new file', 'Select type:', items, 1);
  if (st == nil) then return end;
  ba, sn = Dialogs.InputQuery('Type: ' .. st, 'Enter filename:', false, 'new');
  if (ba == true) then
    -- Read-copy
    fl = io.open(params[2] .. SysUtils.PathDelim .. st, 'rb');
    ct = fl:read('*a');
    fl:close();
    -- Write-paste
    fl = io.open(params[1] .. SysUtils.PathDelim .. string.gsub(st, '(.+)(%.[^%.]+)$', sn .. '%2'), 'wb');
    fl:write(ct);
    fl:close();
  end
end