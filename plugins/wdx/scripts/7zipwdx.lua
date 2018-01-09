-- 7zipwdx.lua

local cmd = "7z"

local properties = {
    {"Type", 8}, 
    {"Method", 8}, 
    {"Solid", 8}, 
    {"Offset", 8}, 
    {"Headers Size", 2},     
    {"Tail Size ", 2},  
    {"Blocks", 1}, 
    {"Streams", 1}, 
    {"Comment", 8}, 
    {"Multivolume", 8}, 
    {"Volumes", 1}, 
    {"Code Page", 8}, 
}

local output = ''
local filename = ''

function ContentGetSupportedField(Index)
    if (properties[Index + 1] ~= nil) then
        return properties[Index + 1][1], '', properties[Index + 1][2]; -- FieldName,Units,ft_string
    elseif (Index == #properties) then
        return "Files", '', 1;
    elseif (Index == #properties + 1) then
        return "Folders", '', 1;
    elseif (Index == #properties + 2) then
        return "Size", '', 2;
    elseif (Index == #properties + 3) then
        return "Compressed", '', 2;        
    elseif (Index == #properties + 4) then
        return "Date", '', 8;
    elseif (Index == #properties + 5) then
        return "Warnings", '', 1;
    elseif (Index == #properties + 6) then
        return "Errors", '', 1; 
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return 'EXT="7Z" | EXT="ZIP" | EXT="RAR" | EXT="XZ" | EXT="GZ" | EXT="Z" | EXT="LZMA" | EXT="BZ2" | EXT="TAR" | EXT="ZIPX"';
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (SysUtils.DirectoryExists(FileName)) then 
        return nil; 
    end
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr > 0) then
            if (math.floor(attr / 0x00000004) % 2 ~= 0)  then
                return nil; 
            end
        end
        local handle = io.popen(cmd .. ' l "' .. FileName .. '" -p');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    
    if (FieldIndex >= 0) and (FieldIndex < #properties) then
        return getproperty(properties[FieldIndex + 1][1])
    elseif (FieldIndex == #properties) then
        return getfromlastlines("(%d+)%sfiles");
    elseif (FieldIndex == #properties + 1) then
        return getfromlastlines("(%d+)%sfolders");
    elseif (FieldIndex == #properties + 2) then
        return getfromlastlines("(%d+)%s+%d+%s+%d+%sf");
    elseif (FieldIndex == #properties + 3) then
        return getfromlastlines("(%d+)%s+%d+%sf");        
    elseif (FieldIndex == #properties + 4) then
        return getfromlastlines("%d%d%d%d%-%d%d%-%d%d%s%d%d:%d%d:%d%d");
    elseif (FieldIndex == #properties + 5) then
        return getfromlastlines("Warnings:%s(%d+)");
    elseif (FieldIndex == #properties + 6) then
        return getfromlastlines("Errors:%s(%d+)");
    end
    return nil; -- invalid
end

function getfromlastlines(pattern)
    local tmp = output:match("%-([%w%s%-:,]+)\n$");
    if tmp ~= nil then
        return tmp:match(pattern);
    end
    return nil;
end

function getproperty(str)
    local tmp = output:match("\n" .. str .. "%s=%s[^\n]+");
    if tmp ~= nil then
        return tmp:gsub("^\n" .. str .. "%s=%s", "");
    end
    return nil;
end