--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{FE3433C9-3980-441C-8D08-CE0FEC98F740}</ID>
    <Icon>cm_symlink</Icon>
    <Hint>Edit Symlink</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$COMMANDER_PATH/scripts/lua/editsymlink.lua</Param>
    <Param>"%p1"</Param>
  </Command>
</doublecmd>

]]

local args = {...};
local path, ret, newpath, msg;
local params = "-n";

local function getOutput(command, stripnewline)
    local handle = io.popen(command, 'r');
    local output = handle:read("*a");
    handle:close();
    if (stripnewline == true) then
        return output:sub(1, -2);
    end
    return output;
end

if (args[1] ~= nil) then
    path = getOutput('readlink ' .. params .. ' "' .. args[1] .. '"', false);
end

if (path ~= nil) and (path ~= "") then
    ret, newpath = Dialogs.InputQuery("Edit Symlink", "Enter new path:", false, path);

    if (ret == true) then
        if (SysUtils.DirectoryExists(newpath)) then
            msg = getOutput('ln -sfn "' .. newpath .. '" "' .. args[1] .. '" 2>&1', true);
        else
            msg = getOutput('ln -sf "' .. newpath .. '" "' .. args[1] .. '" 2>&1', true);
        end

        if (msg ~= nil) and (msg ~= '') then
            Dialogs.MessageBox(msg, "Edit Symlink", 0x0030)
        end
    end
end
