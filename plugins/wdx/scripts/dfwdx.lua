-- dfwdx.lua

local fields = {
    {"Mountpoint", 6, "target | tail -1"},
    {"Target", 8, "target | tail -1"},
    {"Source", 8, "source | tail -1"},
    {"FS Type", 8, "fstype | tail -1"},
    {"Size", 2, "size | tail -1"},
    {"Size (M)", 2, "size -BM | tail -1"},
    {"Size (G)", 2, "size -BG | tail -1"},
    {"Size (T)", 2, "size -BT | tail -1"},
    {"Used", 2, "used | tail -1"},
    {"Used (M)", 2, "used -BM | tail -1"},
    {"Used (G)", 2, "used -BG | tail -1"},
    {"Used (T)", 2, "used -BT | tail -1"},
    {"Available", 2, "avail | tail -1"},
    {"Available (M)", 2, "avail -BM | tail -1"},
    {"Available (G)", 2, "avail -BG | tail -1"},
    {"Available (T)", 2, "avail -BT | tail -1"},
    {"% Used", 2, "pcent | tail -1"},
    {"Inodes", 2, "itotal | tail -1"},
    {"IUsed", 2, "iused | tail -1"},
    {"IFree", 2, "iavail | tail -1"},
    {"% IUsed", 2, "ipcent | tail -1"}
}

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2];
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
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            return nil;
        end
        local handle = io.popen('df "'..FileName..'" --output=' .. fields[FieldIndex + 1][3], 'r');
        local result = handle:read("*a");
        handle:close();
        result = result:sub(1, - 2);
        if (FieldIndex > 3) then
            return result:match("%d+");
        elseif (fields[FieldIndex + 1][1] ~= "Mountpoint") then
            return result;
        elseif (result == FileName) then
            return true;
        end
    end
    return nil; -- invalid
end
