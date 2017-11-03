-- hexheaderwdx.lua

local offset = {
    "0x10", 
    "0x000020", 
    "0x20B0"
}
local sizeLimit = 1000000

function ContentGetSupportedField(Index)
    local submi = '1 byte';
    for i = 2, 16 do
        submi = submi .. '|' .. i .. ' bytes'
    end
    if (Index == 0) then
        return 'header', submi, 8; -- FieldName,Units,ft_string
    elseif (Index == 1) then
        return 'all file(<' .. sizeLimit .. 'b)', '', 8;  -- some problems with ft_fulltext
    elseif (offset[Index - 1] ~= nil) then
        return 'offset: ' .. offset[Index - 1], submi, 8;
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
    if (SysUtils.DirectoryExists(FileName)) then
        return nil;
    end
    local f = io.open(FileName, "rb");
    local bytes = nil;
    if (f ~= nil) then
        if (FieldIndex == 0) then
            bytes = f:read(UnitIndex + 1);
        elseif (FieldIndex == 1) then
            if (f:seek("end") <= sizeLimit) then
                f:seek("set", 0);
                bytes = f:read("*a");
            end
        else 
            f:seek("set", tonumber(offset[FieldIndex - 1]));
            bytes = f:read(UnitIndex + 1);
        end
        f:close();
    end
    if (bytes == nil) then
        return nil; -- invalid
    end
    local t = {}
    for b in string.gfind(bytes, ".") do
        table.insert(t, string.format("%02X ", string.byte(b)));
    end
    local result = table.concat(t);
    return result:sub(1, - 2);
end
