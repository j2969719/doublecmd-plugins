-- This script reads file descriptions from descript.ion
-- luarocks-5.1 install luautf8

local utf8 = require("lua-utf8")

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'Description', 'default|ansi|oem', 8; -- FieldName,Units,ft_string
    elseif (FieldIndex == 1) then
        return 'Description (uppercase filenames)', 'default|ansi|oem', 8; -- FieldName,Units,ft_string
    elseif (FieldIndex == 2) then
        return 'Description (lowercase filenames)', 'default|ansi|oem', 8; -- FieldName,Units,ft_string
    end
    
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FileName:find(SysUtils.PathDelim .. "%.%.$") == nil) then
        local path = FileName:match(".*" .. SysUtils.PathDelim);
        local name = FileName:match("[^" .. SysUtils.PathDelim .. "]+$");
        local case = "";
        if (FieldIndex == 1) then
            case = "upper";
        elseif (FieldIndex == 2) then
            case = "lower";
        end
        if (path ~= nil) and (name ~= nil) then
            local enc = "default";
            if (UnitIndex == 1) then
                enc = "ansi";
            elseif (UnitIndex == 2) then
                enc = "oem";
            end
            return GetDesc(path, name, enc, case);
        else
            return "";
        end
    end
    return nil;
end

function GetDesc(Path, Name, Enc, Case)
    local descfile = GetDFilename(Path);
    if (descfile ~= nil) then
        local f = io.open(Path .. descfile, "r");
        if not f then
            return nil;
        end
        local pattern = GetPattern(Name, Case);
        
        for line in f:lines() do
            local target = LazUtf8.ConvertEncoding(line, Enc, "utf8");
            target = utf8.match(target, "[%w%s%p]+");
            if (target ~= nil) and utf8.find(target, pattern) then
                f:close();
                local result = utf8.gsub(target, pattern, "");
                return result;
            end
        end  
        
        f:close();
    end
    return nil;
end

function GetPattern(Name, Case)
    local magic_chars = {"%", ".", "-", "+", "*", "?", "^", "$", "(", ")", "["};
    local target = Name;
    if (Case == "upper") then
        target = utf8.upper(Name);
    elseif (Case == "lower") then
        target = utf8.lower(Name);
    end
    for k, chr in pairs(magic_chars) do
        target = target:gsub("%" .. chr, "%%%" .. chr);
    end
    return '^"?' .. target .. '"?%s+';
end

function GetDFilename(Path)
    local filename = nil;
    local handle, FindData = SysUtils.FindFirst(Path .. "*");
    if (handle ~= nil) then
        repeat
            if (utf8.lower(FindData.Name) == "descript.ion") or (utf8.upper(FindData.Name) == "FILES.BBS") then
                filename = FindData.Name;
                break;
            end
            result, FindData = SysUtils.FindNext(handle);
        until (result == nil)
        SysUtils.FindClose(handle);
        return filename;
    end
    return nil;
end