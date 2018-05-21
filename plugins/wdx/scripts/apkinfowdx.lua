-- requires aapt

local cmd = "aapt"
local locale = "ru"
local output = ''
local filename = ''

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'Label', '', 8; -- FieldName,Units,ft_string
    elseif (Index == 1) then
        return 'Label (' .. locale .. ')', '', 8;
    elseif (Index == 2) then
        return 'Version', '', 8;
    elseif (Index == 3) then
        return 'Install Location', '', 8;
    elseif (Index == 4) then
        return 'Uses Permissions', '', 8;
    elseif (Index == 5) then
        return 'Locales', '', 8;
    elseif (Index == 6) then
        return 'Supports Screens', '', 8;
    elseif (Index == 7) then
        return 'Native Code', '', 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return 'EXT="APK"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            return nil;
        end
        local handle = io.popen(cmd .. ' dump badging "' .. FileName .. '"');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    
    if (FieldIndex == 0) then
        return getvalue("application%-label:'");
    elseif (FieldIndex == 1) then
        return getvalue("application%-label%-" .. locale .. ":'");
    elseif (FieldIndex == 2) then
        local version = output:match("%sversionName='[^']+");
        if (version ~= nil) then
            return version:gsub("%sversionName='", "");
        else
            return nil;
        end
    elseif (FieldIndex == 3) then
        return getvalue("install%-location:'");
    elseif (FieldIndex == 4) then
        local resultstr = '';
        for permission in string.gmatch(output, "\nuses%-permission: name='([^']+)") do
            if (resultstr ~= '') then
                resultstr = resultstr .. '\n' .. permission;
            else
                resultstr = permission;
            end
        end
        return resultstr;
    elseif (FieldIndex == 5) then
        return getvalue("locales: '", "' '", ', ');
    elseif (FieldIndex == 6) then
        return getvalue("supports%-screens: '", "' '", ', ');
    elseif (FieldIndex == 7) then
        return getvalue("native%-code: '", "' '", ', ');
    end 
    return nil; 
end

function getvalue(str, rpl, by)
    local tmp = output:match("\n" .. str .. "[^\n]+");
    if (tmp ~= nil) then
        tmp = tmp:gsub("^\n" .. str, "");
        if (rpl ~= nil) then
            tmp = tmp:gsub(rpl, by);
        end
        return tmp:gsub("'", "");
    end
    return nil;
end