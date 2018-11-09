--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{58430566-6032-4BCC-986E-FE206ADA40FF}</ID>
    <Icon>cm_markmarkall</Icon>
    <Hint>MarkUnmark</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/scripts/lua/markunmark.lua</Param>
    <Param>%ps2</Param>
  </Command>
</doublecmd>

]]

local params = {...};

if (params[1] == '') then
    DC.ExecuteCommand("cm_MarkMarkAll");
else
    DC.ExecuteCommand("cm_MarkUnmarkAll");
end
