-- dfwdx.lua

local fields = {
    {"Mountpoint", 6, "target | tail -1"}, 
    {"Target", 8, "target | tail -1"}, 
    {"Source", 8, "source | tail -1"}, 
    {"FS Type", 8, "fstype | tail -1"} 
}

function ContentGetSupportedField(Index)
    if (fields[Index + 1] ~= nil ) then
        return fields[Index + 1][1], "", fields[Index + 1][2];
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return nil; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (fields[FieldIndex + 1][3] ~= nil) then
        local handle = io.popen('df "'..FileName..'" --output=' .. fields[FieldIndex + 1][3]);
        local result = handle:read("*a");
        handle:close();
        result = result:sub(1, - 2);
        if (fields[FieldIndex + 1][1] ~= "Mountpoint") then
            return result;
        elseif (result == FileName) then
            return true;
        end
    end
    return nil; -- invalid
end
