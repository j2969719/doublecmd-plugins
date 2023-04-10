--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{FE3433C9-3980-441C-8D08-CE0FEC98F740}</ID>
    <Icon>cm_symlink</Icon>
    <Hint>Edit Symlink</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$DC_CONFIG_PATH/scripts/lua/editsymlink.lua</Param>
    <Param>%"0%p0</Param>
  </Command>
</doublecmd>

]]

local params = {...}

local iattr = SysUtils.FileGetAttr(params[1])
if iattr < 0 then return end
if math.floor(iattr / 0x00000400) % 2 == 0 then return end

local starget = SysUtils.ReadSymbolicLink(params[1], false)
local bres, snewtarget = Dialogs.InputQuery('Edit symlink', 'New target:', false, starget)
if bres == false then return end

local stmp = params[1] .. 'bak1'
local bren, serr, ierr = os.rename(params[1], stmp)
if bren == nil then
  Dialogs.MessageBox('Error: ' .. ierr .. '\n' .. serr, 'Edit symlink')
  return
end
bres = SysUtils.CreateSymbolicLink(snewtarget, params[1])
if bres == false then
  os.rename(stmp, params[1])
  Dialogs.MessageBox('Error: SysUtils.CreateSymbolicLink()', 'Edit symlink')
else
  os.remove(stmp)
end
