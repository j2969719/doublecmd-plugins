-- requires aapt

local cmd = "aapt"
local params = "dump badging"
local locale = "ru"
local separator = "\n"
local output = ''
local filename = ''

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'Label', '', 8; -- FieldName,Units,ft_string
    elseif (FieldIndex == 1) then
        return 'Label (' .. locale .. ')', '', 8;
    elseif (FieldIndex == 2) then
        return 'Package name', '', 8;
    elseif (FieldIndex == 3) then
        return 'Version', '', 8;
    elseif (FieldIndex == 4) then
        return 'Install Location', '', 8;
    elseif (FieldIndex == 5) then
        return 'Uses Permissions', '', 8;
    elseif (FieldIndex == 6) then
        return 'Locales', '', 8;
    elseif (FieldIndex == 7) then
        return 'Supports Screens', '', 8;
    elseif (FieldIndex == 8) then
        return 'Supports Resolutions', '', 8;
    elseif (FieldIndex == 9) then
        return 'Native Code', '', 8;
    elseif (FieldIndex == 10) then
        return 'Minimal SDK', '', 8;
    elseif (FieldIndex == 11) then
        return 'Target SDK', '', 8;
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
        local handle = io.popen(cmd .. ' ' .. params .. ' "' .. FileName:gsub('"', '\\"') .. '"', 'r');
        output = handle:read("*a");
        handle:close();
        if (output == nil) then
            return nil;
        end
        filename = FileName;
    end
    if (FieldIndex == 0) then
        local tmp = getvalue("application%-label:'");
        if (tmp == nil) then
            return output:match("\napplication: label='([^']+)");
        end
    elseif (FieldIndex == 1) then
        return getvalue("application%-label%-" .. locale .. ":'");
    elseif (FieldIndex == 2) then
        return output:match("package: name='([^']+)");
    elseif (FieldIndex == 3) then
        local version = output:match("%sversionName='[^']+");
        if (version ~= nil) then
            return version:gsub("%sversionName='", "");
        else
            return nil;
        end
    elseif (FieldIndex == 4) then
        return getvalue("install%-location:'");
    elseif (FieldIndex == 5) then
        local resultstr = '';
        for permission in string.gmatch(output, "\nuses%-permission: name='([^']+)") do
            if (resultstr ~= '') then
                resultstr = resultstr .. separator .. permission;
            else
                resultstr = permission;
            end
        end
        return resultstr;
    elseif (FieldIndex == 6) then
        return getvalue("locales: '", "' '", ', ');
    elseif (FieldIndex == 7) then
        return getvalue("supports%-screens: '", "' '", ', ');
    elseif (FieldIndex == 8) then
        return getvalue("densities: '", "' '", ', ');
    elseif (FieldIndex == 9) then
        return getvalue("native%-code: '", "' '", ', ');
    elseif (FieldIndex == 10) then
        return getvalue("sdkVersion:'");
    elseif (FieldIndex == 11) then
        return getvalue("targetSdkVersion:'");
    end
    return nil;
end

function getvalue(str, rpl, by)
    local tmp = output:match("\n" .. str .. "([^\n]+)");
    if (tmp ~= nil) then
        if (rpl ~= nil) then
            tmp = tmp:gsub(rpl, by);
        end
        return tmp:gsub("'", "");
    end
    return nil;
end
