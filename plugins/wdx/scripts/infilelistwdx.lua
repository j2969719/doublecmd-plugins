
--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{5C4C302C-D24F-42BC-B8E5-EA7F6E784FF0}</ID>
    <Icon>cm_pastefromclipboard</Icon>
    <Hint>Add selected in filelist.txt</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>/path/to/infilelistwdx.lua</Param>
    <Param>%LU</Param>
  </Command>
</doublecmd>

]]

local args = {...}

local filelist = "filelist.txt";
local homedir = os.getenv("HOME");

if (homedir ~= nil) then
    filelist = homedir .. "/.config/doublecmd/" .. filelist;
else
    filelist = "/tmp/_doublecmd-" .. filelist;
end

if (#args == 1) then
    local file = io.open(filelist, "a");
    if (file ~= nil) then
        local list = io.open(args[1], "r");
        if (list ~= nil) then
            io.output(file);
            for line in list:lines() do
                io.write(line .. '\n');
            end
            list:close();
        end
        file:close();
    end
end

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'in ' .. filelist, '', 6; -- FieldName,Units,ft_boolean
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FieldIndex == 0) then
        local found = false;
        local file = io.open(filelist, "r");
        if (file ~= nil) then
            for line in file:lines() do
                if (line == FileName) then
                    found = true;
                    break;
                end

            end
            file:close();
            return found;
        end
    end
    return nil; -- invalid
end