-- simplefileinfo.lua

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'File type (file -b)', '', 8; -- FieldName,Units,ft_string
    elseif (Index == 1) then
        return 'Access rights in octal (stat -c%a)', '', 1;
    elseif (Index == 2) then
        return 'Access rights', 'd|l|p|r(user)|w(user)|x(user)|r(group)|w(group)|x(group)|r(other)|w(other)|x(other)', 6;
    elseif (Index == 3) then
        return 'User ID (stat -c%u)', '', 1;
    elseif (Index == 4) then
        return 'User name (stat -c%U)', '', 8;
    elseif (Index == 5) then
        return 'Group ID (stat -c%g)', '', 1;
    elseif (Index == 6) then
        return 'Group name (stat -c%G)', '', 8;
    elseif (Index == 7) then
        return 'CRC32', '', 8;
    elseif (Index == 8) then
        return 'MD5', '', 8;
    elseif (Index == 9) then
        return 'SHA256', '', 8;
    elseif (Index == 10) then
        return 'MIME type', '', 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (string.sub(FileName, - 3) == "/..") or (string.sub(FileName, - 2) == "/.") then
        return nil;
    end
    if (FieldIndex == 0) then
        local handle = io.popen('file -b "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        local result = result:sub(1, - 2);
        return result;
    elseif (FieldIndex == 1) then
        local handle = io.popen('stat --printf=%a "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        return result;
    elseif (FieldIndex == 2) then
        local handle = io.popen('stat --printf=%A "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        if (UnitIndex == 0) and (string.sub(result,1,1) == 'd') then
            return true;
        elseif (UnitIndex == 1) and (string.sub(result,1,1) == 'l') then
            return true;
        elseif (UnitIndex == 2) and (string.sub(result,1,1) == 'p') then
            return true;
        elseif (UnitIndex == 3) and (string.sub(result,2,2) == 'r') then
            return true;
        elseif (UnitIndex == 4) and (string.sub(result,3,3) == 'w') then
            return true;
        elseif (UnitIndex == 5) and (string.sub(result,4,4) == 'x') then
            return true;
        elseif (UnitIndex == 6) and (string.sub(result,5,5) == 'r') then
            return true;
        elseif (UnitIndex == 7) and (string.sub(result,6,6) == 'w') then
            return true;
        elseif (UnitIndex == 8) and (string.sub(result,7,7) == 'x') then
            return true;
        elseif (UnitIndex == 9) and (string.sub(result,8,8) == 'r') then
            return true;
        elseif (UnitIndex == 10) and (string.sub(result,9,9) == 'w') then
            return true;
        elseif (UnitIndex == 11) and (string.sub(result,10,10) == 'x') then
            return true;
        end
        return nil;
    elseif (FieldIndex == 3) then
        local handle = io.popen('stat --printf=%u "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        return result;
    elseif (FieldIndex == 4) then
        local handle = io.popen('stat --printf=%U "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        return result;
    elseif (FieldIndex == 5) then
        local handle = io.popen('stat --printf=%g "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        return result;
    elseif (FieldIndex == 6) then
        local handle = io.popen('stat --printf=%G "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        return result;
    elseif (FieldIndex == 7) then
        if (flags == 1) then return nil end
        local handle = io.popen('crc32 "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        local result = result:sub(1, - 2);
        return result;
    elseif (FieldIndex == 8) then
        if (flags == 1) then return nil end
        local handle = io.popen('md5sum "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        local result = string.match(result, '(%w+)%s+');
        return result;
    elseif (FieldIndex == 9) then
        if (flags == 1) then return nil end
        local handle = io.popen('sha256sum "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        local result = string.match(result, '(%w+)%s+');
        return result;
    elseif (FieldIndex == 10) then
        local handle = io.popen('file -b --mime-type "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        local result = result:sub(1, - 2);
        return result;
    end
    return nil; -- invalid
end
