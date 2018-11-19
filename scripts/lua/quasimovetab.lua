--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{5CF4F955-29F7-41B1-AD5C-6B991A099DFC}</ID>
    <Icon>cm_newtab</Icon>
    <Hint>Move tab to opposite side</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$COMMANDER_PATH/scripts/lua/quasimovetab.lua</Param>
    <Param>%D</Param>
  </Command>
</doublecmd>

]]

local params = {...};

DC.ExecuteCommand("cm_CloseTab");
DC.ExecuteCommand("cm_FocusSwap");
DC.ExecuteCommand("cm_NewTab");
DC.ExecuteCommand("cm_ChangeDir", params[1]);