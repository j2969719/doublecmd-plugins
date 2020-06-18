--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{7D91C086-852A-4E1A-9663-896B44179F56}</ID>
    <Icon>cm_changedir</Icon>
    <Hint>whoop whoop whoop</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$COMMANDER_PATH/scripts/lua/chdirr.lua</Param>
    <Param>%Ds</Param>
    <Param>%Dt</Param>
    <Param>noswap</Param>
  </Command>
</doublecmd>

]]

local args = {...};
local dirs = {};
local add = '';
local current = args[1];
local target = args[2];
local panel = 'activepath';
local options = '';

function moredelims(current, target)
    local _current = current:gsub('\\ ', ' ');
    local _target = target:gsub('\\ ', ' ');
    local _, c_count = _current:gsub(SysUtils.PathDelim, '');
    local _, t_count = _target:gsub(SysUtils.PathDelim, '');

    if (c_count > t_count) then
        return true;
    else
        return false;
    end
end

if (current ~= target) then
    if (args[3] ~= nil) then
        options = args[3];
    end

    if moredelims(current, target) and not options:find("noswap") then
        current = args[2];
        target = args[1];
        panel = 'inactivepath';
    end

    for dir in target:gmatch("([^" .. SysUtils.PathDelim .. "]+)") do
        table.insert(dirs, 1, dir);
    end

    for i = 1, #dirs do
        add = SysUtils.PathDelim .. dirs[i] .. add;
        local path = current .. add;
        if (SysUtils.DirectoryExists(path:gsub('\\ ', ' '))) then
            DC.ExecuteCommand("cm_ChangeDir", panel .. '=' .. path);
            if (options:find("first match")) then
                break;
            end
        end
    end
end