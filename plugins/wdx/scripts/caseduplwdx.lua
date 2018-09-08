-- dc >= r8316

local dupl_found = "Yes"
local dupl_notfound = "-"

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return "case duplicates", dupl_found .. '|' .. dupl_notfound, 7;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FieldIndex == 0) then
        local IsFound = dupl_notfound;
        if (FileName:find("[" .. SysUtils.PathDelim .. "]%.%.$")) then
            return nil;
        end
        local CurrentName = FileName:match("[" .. SysUtils.PathDelim .. "]([^" .. SysUtils.PathDelim .. "]+)$");
        local CurrentPath = FileName:match("(.*[" .. SysUtils.PathDelim .. "])");
        local Handle, FindData = SysUtils.FindFirst(CurrentPath .. "*");
        if (Handle ~= nil) then
            repeat
                if (FindData.Name ~= CurrentName) and (LazUtf8.LowerCase(CurrentName) == LazUtf8.LowerCase(FindData.Name)) then
                    IsFound = dupl_found;
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