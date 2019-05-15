-- !!! editing file in doublecmd via admin:// will change the ownership from root to user !!!


--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{0B677082-AE10-46F2-826C-503C64A52FCD}</ID>
    <Icon>system-run-symbolic</Icon>
    <Hint>open current dir as admin</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/jumpasadmin.lua</Param>
    <Param>%D</Param>
  </Command>
</doublecmd>

]]

local args = {...};
local path = "admin://";
if (args[1] ~= nil) then
    path = path .. args[1];
    path = path:gsub(' ', '\\ ');
end
DC.ExecuteCommand("cm_ChangeDir", path);
