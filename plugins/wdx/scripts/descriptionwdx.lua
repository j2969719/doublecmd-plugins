-- This script reads file descriptions from descript.ion

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'Description', 'default|ansi|oem', 8; -- FieldName,Units,ft_string
    elseif (FieldIndex == 1) then
        return 'Description (uppercase filenames)', 'default|ansi|oem', 8; -- FieldName,Units,ft_string
    elseif (FieldIndex == 2) then
        return 'Description (lowercase filenames)', 'default|ansi|oem', 8; -- FieldName,Units,ft_string
    elseif (FieldIndex == 3) then
        return 'Description (uppercase and lowercase filenames)', 'default|ansi|oem', 8; -- FieldName,Units,ft_string
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
        elseif (FieldIndex == 3) then
            case = "both";
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
        if (f ~= nil) then
            local result = nil;
            if (Case == "both") then
                result = CheckLines(f, GetPattern(Name, "upper"), Enc);
                if (result == nil) then
                    f:seek("set", 0);
                    result = CheckLines(f, GetPattern(Name, "lower"), Enc);
                end
            else
                result = CheckLines(f, GetPattern(Name, Case), Enc);
            end
            f:close();
            return result;
        end
    end
    return nil;
end

function GetPattern(Name, Case)
    local magic_chars = {"%", ".", "-", "+", "*", "?", "^", "$", "(", ")", "["};
    local target = Name;
    if (Case == "upper") then
        target = LazUtf8.UpperCase(Name);
    elseif (Case == "lower") then
        target = LazUtf8.LowerCase(Name);
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
            if (LazUtf8.LowerCase(FindData.Name) == "descript.ion") or (LazUtf8.UpperCase(FindData.Name) == "FILES.BBS") then
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

function CheckLines(File, Pattern, Enc)
    local firstline = true;
    local target = "";
    for line in File:lines() do
        if (firstline == true) then
            firstline = false;
            if (Enc ~= "ansi") and (Enc ~= "oem") and (line:sub(1, 3) == "\239\187\191") then
                target = line:sub(4);
            else
                target = line;
            end
        else
            target = line;
        end
        if target:find(Pattern) then
            local result = target:gsub(Pattern, "") .. CheckNextLines(File);
            return LazUtf8.ConvertEncoding(result, Enc, "utf8");
        end
    end
    return nil;
end

function CheckNextLines(File)
    local pos = File:seek();
    local result = "";
    for line in File:lines() do
        if line:find("^%s+|%s+") then
            result = result .. "\n" .. line:gsub("^%s+|%s+", "");
        else
            break;
        end
        pos = File:seek();
    end
    File:seek("set", pos);
    return result;
end