
local settings = {
    ['zip' ] = {
        'unzip -z "$FILE"',       -- command
        "^Archive[^\n]+\n(.+)\n$" -- pattern
        }, 
    
    ['rar' ] = {
        'rar l "$FILE" -p-', 
        "Type 'rar %-%?' for help\n\n(.+)\n\n\nArchive: "
        }, 
}

local notfoundstr = "not found"

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return "comment", 'default|show "' .. notfoundstr .. '"', 8; -- FieldName,Units,ft_string
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    local detect_string = '';
    for ext in pairs(settings) do
        if (detect_string == '') then
            detect_string = '(EXT="' .. ext:upper() .. '")';
        else
            detect_string = detect_string .. ' | (EXT="' .. ext:upper() .. '")';
        end
    end
    return detect_string; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if not SysUtils.DirectoryExists(FileName) and (FieldIndex == 0) then
        local result = nil;
        local ext = FileName:match(".+%.(.+)$");
        if (ext ~= nil) then
            ext = ext:lower();
            if (settings[ext] ~= nil) then
                local handle = io.popen(settings[ext][1]:gsub("$FILE", FileName), 'r');
                local output = handle:read("*a");
                handle:close();
                if (output ~= nil) then
                    result = output:match(settings[ext][2]);
                end
            end
        end
        if (UnitIndex == 1) and (result == nil) then
            return notfoundstr;
        else
            return result;
        end
    end
    return nil; -- invalid
end
