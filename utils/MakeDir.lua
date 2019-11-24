-- MakeDir.lua (cross-platform)
-- 2019.11.24
--
-- Creating directories with additional features.
-- You can use absolute name (/path/to/newdir), relative to current path (to/newdir) or only name (newdir).
-- Script will also create all directories leading up to the entered name(s) that do not exist already.
-- If you want more one dir at once then use vertical bar:
--   newdir1|newdir2|newdir3
--
-- Params:
--   %"0%D
--   %permission_mode%
--
-- %permission_mode% is optional parameter for Linux only, use octal mode (for example, 755 or 777)
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

local function path_is_absolute(s)
  if SysUtils.PathDelim == '\\' then
    if string.match(s, '^[A-Za-z]:\\.*') ~= nil then return true end
  else
    if string.sub(s, 1, 1) == '/' then return true end
  end
  return false
end

local function make_list(s, p, ds)
  local l
  local a = {}
  for l in string.gmatch(s, '[^|]+') do
    if path_is_absolute(l) then
      table.insert(a, l)
    else
      table.insert(a, p .. ds .. l)
    end
  end
  if ds == '\\' then
    return a
  else
    return '"' .. table.concat(a, '" "') .. '"'
  end
end

local function get_output(command)
  local handle = io.popen(command, 'r')
  local output = handle:read('*a')
  handle:close()
  return output:sub(1, -2)
end

local params = {...}

if (#params == 0) or (#params > 2) then
  Dialogs.MessageBox('Check the number of parameters!', 'Make dir(s)', 0x0030)
  return
end
local msg
local ba, dn = Dialogs.InputQuery('Make dir(s)', 'Enter name:', false, 'new')
if ba == true then
  if SysUtils.PathDelim == '/' then
    local cmd = 'mkdir -p'
    if #params == 2 then cmd = 'mkdir -m ' .. params[2] .. ' -p' end
    if string.find(dn, '|') ~= nil then
      cmd = cmd .. ' ' .. make_list(dn, params[1], '/') .. ' 2>&1'
    else
      if path_is_absolute(dn) == false then dn = params[1] .. '/' .. dn end
      cmd = cmd .. ' "' .. dn .. '" 2>&1'
    end
    msg = get_output(cmd)
    if (msg ~= nil) and (msg ~= '') then
      Dialogs.MessageBox(msg, 'Make dir(s)', 0x0030)
    end
  else
    if string.find(dn, '|') ~= nil then
      local nd = make_list(dn, params[1], '\\')
      local tmp
      msg = 'Error:'
      if #nd > 0 then
        for i = 1, #nd do
          tmp = SysUtils.CreateDirectory(nd[i])
          if tmp == false then
            msg = msg .. '\n' .. nd[i]
          end
        end
        if string.len(msg) > 8 then Dialogs.MessageBox(msg, 'Make dir(s)', 0x0030) end
      else
        Dialogs.MessageBox('Unknown error.', 'Make dir(s)', 0x0030)
      end
    else
      if path_is_absolute(dn) == false then dn = params[1] .. '\\' .. dn end
      if SysUtils.CreateDirectory(dn) ~= true then
        Dialogs.MessageBox('Error:\n' .. dn, 'Make dir(s)', 0x0030)
      end
    end
  end
end
