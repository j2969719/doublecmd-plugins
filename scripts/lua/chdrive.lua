--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{DA532500-1050-44B1-B5E6-5C193F2C8FEB}</ID>
    <Icon>cm_changedir</Icon>
    <Hint>dive drive</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$DC_CONFIG_PATH/scripts/lua/chdrive.lua</Param>
    <Param>%"0%D</Param>
    <Param>options: try_parrent,log,inactive_panel</Param>
    <Param>choice: sda2;sdb1</Param>
  </Command>
</doublecmd>


]]

args = {...}
folder  = args[1]
options = args[2]
drives  = args[3]

function get_output(command)
    local output = nil
    local handle = io.popen(command, 'r')
    if handle then
        output = handle:read("*a")
        handle:close()
    end
    return output
end

local panel = "activepath"
local try_parrent = false
local log_err = false

if options ~= nil then
    if options:find("inactive_panel") then
        panel = "inactivepath"
    end
    if options:find("try_parrent") then
        try_parrent = true
    end
    if options:find("log") then
        log_err = true
    end
end

if folder ~= nil then
    local path = folder .. SysUtils.PathDelim
    local prefix_len = 3
    local result = nil
    if drives ~= nil then
        local first, second = drives:match("choice:%s+([^;]+);(.+)")
        if not first or not second then
            Dialogs.MessageBox("Enter two drives separated by a semicolom.", "", 0x0010)
        else
            if SysUtils.PathDelim == '/' then
               first = get_output('df --output=target /dev/' .. first .. ' | tail -1'):sub(1, -2)
               second = get_output('df --output=target /dev/' .. second .. ' | tail -1'):sub(1, -2)
            end
            if not first or first == '' then
                Dialogs.MessageBox("Failed to get mountpoint for " .. drives:match("choice:%s+([^;]+);.+") .. ".", "", 0x0010)
            elseif not second or second == '' then
                Dialogs.MessageBox("Failed to get mountpoint for " .. drives:match("choice:%s+[^;]+;(.+)") .. ".", "", 0x0010)
            else
                if first ~= '/' and path:sub(1, first:len()) == first then
                    prefix_len = first:len() + 1
                    result = second
                elseif path:sub(1, second:len()) == second then
                    prefix_len = second:len() + 1
                    result = first
                elseif first == '/' then
                    prefix_len = first:len() + 1
                    result = second
                end
            end
        end
    elseif SysUtils.PathDelim == '/' then
        local output = get_output('df --output=target -x devtmpfs | tail -n +2')
        if output ~= nil then
            local mounts = {}
            for line in output:gmatch("[^\n]-\n") do
                table.insert(mounts, 1, line:sub(1, -2))
            end
            local mountpoint = get_output('df --output=target "' .. path:gsub('"', '\\"') .. '" | tail -1'):sub(1, -2)
            prefix_len = mountpoint:len() + 1
            result = Dialogs.InputListBox("", "Select mountpoint", mounts, mountpoint)
        end
    else
        local drives = {}
        for i = 65, 90 do
            table.insert(drives, string.char(i) .. ':')
        end
        result = Dialogs.InputListBox("", "Select drive", drives, string.upper(SysUtils.ExtractFileDrive(path)))
    end
    if result ~= nil then
        local target = path:sub(prefix_len)
        if target ~= nil then
            if target:sub(1,1) ~= SysUtils.PathDelim then
                target = SysUtils.PathDelim .. target
            end
            path = result .. target
            if SysUtils.DirectoryExists(path) then
                DC.ExecuteCommand("cm_ChangeDir", panel .. '=' .. path)
            elseif try_parrent then
                while (not SysUtils.DirectoryExists(path)) do
                    path = path:match("(.+" .. SysUtils.PathDelim ..")[^" .. SysUtils.PathDelim .. "]-" .. SysUtils.PathDelim .. "$")
                    if not path then
                        break
                    end
                    if log_err then
                        DC.LogWrite(path .. " is not exists.", 2, true, false)
                    end
                end
                if path ~= nil then
                    DC.ExecuteCommand("cm_ChangeDir", panel .. '=' .. path)
                else
                    if log_err then
                        DC.LogWrite("Failed to change directory.", 2, true, false)
                    else
                        Dialogs.MessageBox("Failed to change directory.", "", 0x0010)
                    end
                end
            else
                if log_err then
                    DC.LogWrite(path .. " is not exists.", 2, true, false)
                else
                    Dialogs.MessageBox(path .. " is not exists.", "", 0x0010)
                end
            end
        end
    end
end