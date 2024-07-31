--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{58430566-6032-4BCC-986E-FE206ADA40FF}</ID>
    <Icon>cm_markmarkall</Icon>
    <Hint>MarkUnmark</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$DC_CONFIG_PATH/scripts/lua/markunmark.lua</Param>
  </Command>
</doublecmd>

]]

local function IsSelectionExists()
  local sTmp
  local sFile = os.tmpname()
  DC.ExecuteCommand("cm_SaveSelectionToFile", sFile)
  local h = io.open(sFile, "rb")
  if h ~= nil then
    sTmp = h:read(1)
    h:close()
  end
  os.remove(sFile)
  if (sTmp == nil) or (sTmp == "") then
    return false
  else
    return true
  end
end

if IsSelectionExists() == false then
  DC.ExecuteCommand("cm_MarkMarkAll")
else
  DC.ExecuteCommand("cm_MarkUnmarkAll")
end
