--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{4168DECC-B25D-4621-8B9D-A0E44DAF1397}</ID>
    <Icon>cm_rename</Icon>
    <Hint>Copy and remove</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$DC_CONFIG_PATH/scripts/lua/copydelete.lua</Param>
  </Command>
</doublecmd>


]]

local message = "just do it make your dreams come true"

local ret = Dialogs.MessageBox(message, "", 0x0024)
if ret == 0x0006 then
    DC.ExecuteCommand("cm_SaveSelection")
    DC.ExecuteCommand("cm_CopyNoAsk")
    DC.ExecuteCommand("cm_RestoreSelection")
    DC.ExecuteCommand("cm_Delete", "confirmation=0", "trashcan=1")
end