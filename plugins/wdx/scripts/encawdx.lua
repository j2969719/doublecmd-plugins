-- requires enca

local cmd = "enca"
local params = ''
local output = ''
local filename = ''

local fields = {
    {"iconv name",           "-g -i",    "([^\n]+)"},   -- display name, command line parameters, pattern
    {"enca's encoding name", "-g -e",    "([^\n]+)"},
    {"lang ru",              "-L ru -g",  "(.+)\n$"},
}


function ContentGetSupportedField(Index)
    if (fields[Index + 1] ~= nil) then
        return fields[Index + 1][1], "", 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return nil; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (filename ~= FileName) or (params ~= fields[FieldIndex + 1][2]) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr > 0) then
            if (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
                return nil; 
            end
        end
        local handle = io.popen(cmd .. ' ' .. fields[FieldIndex + 1][2] .. ' "' .. FileName .. '"');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
        params = fields[FieldIndex + 1][2];
    end
    if (fields[FieldIndex + 1][3] ~= nil) then
        return output:match(fields[FieldIndex + 1][3]);
    end 
    return nil;
end
