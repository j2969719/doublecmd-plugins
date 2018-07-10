local utf8 = require("lua-utf8")

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'case duplicates', '', 6;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FieldIndex == 0) then
        local IsFound = false;
        local DelimPattern = SysUtils.PathDelim;
        if (DelimPattern == nil) then
            DelimPattern = "/\\";
        end
        if (FileName:find("[" .. DelimPattern .. "]%.%.$")) then
            return nil;
        end
        local CurrentPath = FileName:match("(.*[" .. DelimPattern .. "])");
        local CurrentName = FileName:match("[" .. DelimPattern .. "]([^" .. DelimPattern .. "]+)$");
        local Handle, FindData = SysUtils.FindFirst(CurrentPath .. "*");
        if (Handle ~= nil) then
            repeat
                if (FindData.Name ~= CurrentName) and (utf8.lower(CurrentName) == utf8.lower(FindData.Name)) then
                    IsFound = true;
                    break;
                end
                Result, FindData = SysUtils.FindNext(Handle)
            until (Result == nil)
            SysUtils.FindClose(Handle)
            return IsFound;
        end
    end
    return nil; -- invalid
end