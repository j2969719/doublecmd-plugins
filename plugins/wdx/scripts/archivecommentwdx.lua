
local settings = { -- command, additional parameters, pattern, ext, ...  
    {"unzip -z", '',    "^Archive[^\n]+\n(.+)\n$",                         "zip"}, 
    {"rar l",    '-p-', "Type 'rar %-%?' for help\n\n(.+)\n\n\nArchive: ", "rar"}, 
} 
local notfoundstr = "not found"

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return "comment", 'default|show "' .. notfoundstr .. '"', 8; -- FieldName,Units,ft_string
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    local detectstring = '';
    for arc = 1, #settings do
        for i = 4, #settings[arc] do
            if (detectstring ~= '') then
                detectstring = detectstring .. ' | (EXT="' .. string.upper(settings[arc][i]) .. '")';
            else
                detectstring = '(EXT="' .. string.upper(settings[arc][i]) .. '")';
            end
        end
    end
    return detectstring; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FieldIndex == 0) then
        local result = nil;
        local ext = FileName:match(".+%.(.+)$");
        for arc = 1, #settings do
            for i = 4, #settings[arc] do
                if (ext == settings[arc][i]) then
                    local handle = io.popen(settings[arc][1] .. ' "' .. FileName .. '" ' .. settings[arc][2]);
                    local output = handle:read("*a");
                    handle:close();
                    if (output ~= nil) then
                        result = output:match(settings[arc][3]);
                    end
                    break;
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
