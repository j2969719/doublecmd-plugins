local cmd = "tar -tvf"
local filename = ''
local files = 0;
local folders = 0; 
local symlinks = 0; 

local fields = {
    {"files",   2}, 
    {"folders", 2}, 
    {"symlinks", 2}, 
}

function ContentGetSupportedField(Index)
    if (fields[Index + 1] ~= nil ) then
        return fields[Index + 1][1], "", fields[Index + 1][2];
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return '(EXT="TAR")|(EXT="GZ")|(EXT="BZ2")|(EXT="LZMA")|(EXT="XZ")|(EXT="TGZ")|(EXT="TBZ")|(EXT="TBZ2")|(EXT="TZMA")|(EXT="TZX")|(EXT="TLZ")';
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            return nil;
        end
        files = 0;
        folders = 0; 
        symlinks = 0; 
        local handle = io.popen(cmd .. ' "' .. FileName .. '" ', 'r');
        local output = handle:read("*a");
        handle:close();
        if (output == '') or (output == nil) then 
            return nil;
        end 
        filename = FileName;
        for line in output:gmatch("([^\n]*)\n?") do
            if (line:find("^d[^\n]+")) then 
                folders = folders + 1;
            end
            if (line:find("^%-[^\n]+")) then
                files = files + 1;
            end
            if (line:find("^l[^\n]+")) then
                symlinks = symlinks + 1;
            end
        end
        
    end
    
    if (FieldIndex == 0) then
        return files;
    elseif (FieldIndex == 1) then 
        return folders;
    elseif (FieldIndex == 2) then 
        return symlinks;
    end
    return nil; -- invalid
end