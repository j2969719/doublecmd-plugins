-- emptydirwdx.lua

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'is empty', '', 6;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return nil;
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local empty = true;
    if not (SysUtils.DirectoryExists(FileName)) then 
        return nil; 
    end
    if (string.find(FileName, "[^" .. SysUtils.PathDelim .. "]%.%.$")) then
        return nil;
    end
    local Handle, FindData = SysUtils.FindFirst(FileName .. SysUtils.PathDelim .. "*")
    if Handle ~= nil then
        repeat    
            if ((FindData.Name ~= ".") and (FindData.Name ~= "..")) then
                empty = false;
                break;
            end       
            Result, FindData = SysUtils.FindNext(Handle)
            if (Result ~= nil) and (FindData.Name ~= ".") and (FindData.Name ~= "..") then
                empty = false;
                break;
            end
        until (Result == nil)
    else
        return nil;
    end
    SysUtils.FindClose(Handle);
    return empty;
end
