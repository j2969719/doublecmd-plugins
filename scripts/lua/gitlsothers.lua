--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{749C818A-D1E2-4916-9FB1-51D9FC8B8497}</ID>
    <Icon>cm_loadlist</Icon>
    <Hint>git ls others</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$DC_CONFIG_PATH/scripts/lua/gitlsothers.lua</Param>
    <Param>%D</Param>
    <Param>git ls-files -o</Param>
  </Command>
</doublecmd>

]]

local args = {...}
local filelist = '/tmp/_dctemplist.lst'
local gitpath = args[1]
local gitcmd = args[2]

if (gitpath ~= nil and gitcmd ~= nil) then
    os.execute('cd "' .. gitpath:gsub('"', '\\"') .. '" && ' .. gitcmd .. ' | sed "s~^~' .. gitpath .. '/~" > "' .. filelist:gsub('"', '\\"') .. '"')
    local file = io.open(filelist, 'r')
    if (file ~= nil) then
        local filesize = file:seek("end")
        file:close()
        if (filesize > 0) then
            DC.ExecuteCommand("cm_LoadList", "filename=" .. filelist)
        end
    end
end
