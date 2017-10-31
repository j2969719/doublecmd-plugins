-- ... please do not try this at home

local fields = {
    {"first name", 8, "first%-name"}, 
    {"middle name", 8, "middle%-name"}, 
    {"last name", 8, "last%-name"}, 
    {"book title", 8, "book%-title"}, 
    {"publisher", 8, "publisher"}, 
    {"city", 8, "city"}, 
    {"year", 2, "year"}
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
    return '(EXT="XML")|(EXT="FB2")'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (SysUtils.DirectoryExists(FileName)) then
        return nil;
    end
    if (fields[FieldIndex + 1][3] ~= nil ) then
        local file = io.open(FileName, "r");
        if file then
            for line in file:lines() do
                if string.find(line, '<' .. fields[FieldIndex + 1][3] .. '>') then
                    file:close();
                    local result = string.gsub(line, '<' .. fields[FieldIndex + 1][3] .. '>' , "");
                    return result:gsub('</' .. fields[FieldIndex + 1][3] .. '>', "");
                end
            end
            file:close();
        end
    end
    return nil;
end
