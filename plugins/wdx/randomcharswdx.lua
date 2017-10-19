-- returns a fixed number of random characters for auto-renaming purposes

local charset = {}
local filename = ""
local count = 10

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return "1 character", "", 8;
        
    elseif (Index < count) then
        return Index + 1 .. " characters", "", 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return ''; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local str = '';
    if (filename ~= FileName) then
        for i = 48, 57 do
            table.insert(charset, string.char(i));
        end
        for i = 65, 90 do
            table.insert(charset, string.char(i)); 
        end
        for i = 97, 122 do 
            table.insert(charset, string.char(i));
        end
        filename = FileName;
    end
    math.randomseed(os.time());
    if (FieldIndex >= 0 ) and (FieldIndex < count ) then
        for i = 1, FieldIndex + 1 do
            str = str .. charset[math.random(1, #charset)];
        end
        return str;
    end
    return nil; -- invalid
end
