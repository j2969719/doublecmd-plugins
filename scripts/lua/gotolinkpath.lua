--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{4651FC8B-CC8A-4D8B-8741-DB29D812B7E3}</ID>
    <Icon>cm_symlink</Icon>
    <Hint>go to link path</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$COMMANDER_PATH/scripts/lua/gotolinkpath.lua</Param>
    <Param>"%p1"</Param>
    <Param>onlydir</Param>
  </Command>
</doublecmd>

]]

local args = {...};
local path;
local params = "-m"

if (args[1] ~= nil) then
    local handle = io.popen('readlink ' .. params .. ' "' .. args[1] .. '"', 'r');
    local output = handle:read("*a");
    handle:close();
    if (string.find(output, "\n")) then
        path = string.match(output, "(.*[\n])");
    else
        path = output;
    end
    if (args[2] == "onlydir") and (SysUtils.DirectoryExists(path) == false) then
        path = string.match(output, "(.*[/\\])");
    end
end

if (path ~= nil) then
    path = path:gsub(' ', '\\ ');
    DC.ExecuteCommand("cm_ChangeDir", path);
end