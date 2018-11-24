-- Make dir(s)
-- 2018.11.22
--
-- Now Unix-like OS only.
-- You can use absolute name (/path/to/newdir), relative to current path (to/newdir) or only name (newdir).
-- Script will also create all directories leading up to the entered name(s) that do not exist already.
-- If you want more one dir at once then use vertical bar:
--   name1|name2|name3
--
-- Params:
--   %"0%D
--   %permission_mode%
--
-- %permission_mode% is optional parameter, use octal mode (for example, 755 or 777)
--
-- Button example:
--[[
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{78AE07D0-42A0-4238-92BB-BE802AFB6F49}</ID>
    <Icon>cm_executescript</Icon>
    <Hint>Make dir(s)</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/utils/MakeDir.lua</Param>
    <Param>%"0%D</Param>
  </Command>
</doublecmd>
]]

local function path_is_absolute(n)
  if (string.sub(n, 1, 1) == SysUtils.PathDelim) then return true end;
  return false;
end

local function make_one_string(s, p)
  local l, r
  local a = {}
  for l in string.gmatch(s, '[^|]+') do
    if path_is_absolute(l) then
      table.insert(a, l);
    else
      table.insert(a, p .. SysUtils.PathDelim .. l);
    end
  end
  r = '"' .. table.concat(a, '" "') .. '"';
  return r;
end

local function get_output(command)
  local handle = io.popen(command, 'r');
  local output = handle:read('*a');
  handle:close();
  return output:sub(1, -2);
end

local params = {...}
local cmd, ba, dn, nd, cmdl, msg

if (#params > 2) then
  Dialogs.MessageBox('Check the number of parameters!\nRequires only two.', 'Make dir(s)', 0x0030);
  return;
end
if (#params == 2) then
  cmd = 'mkdir -m ' .. params[2] .. ' -p';
else
  cmd = 'mkdir -p';
end
ba, dn = Dialogs.InputQuery('Make dir(s)', 'Enter name:', false, 'new');
if (ba == true) then
  if (string.find(dn, '|') ~= nil) then
    nd = make_one_string(dn, params[1]);
    cmdl = cmd .. ' ' .. nd .. ' 2>&1';
  else
    if path_is_absolute(dn) then
      cmdl = cmd .. ' "' .. dn .. '" 2>&1';
    else
      cmdl = cmd .. ' "' .. params[1] .. SysUtils.PathDelim .. dn .. '" 2>&1';
    end
  end
  msg = get_output(cmdl);
  if (msg ~= nil) and (msg ~= '') then
    Dialogs.MessageBox(msg, 'Make dir(s)', 0x0030);
  end
end
