-- filenamechrstatwdx.lua
-- luarocks-5.1 install luautf8

local utf8 = require("lua-utf8")
local DelimPattern = SysUtils.PathDelim;
local UnitsStr = "Filename|Filename.Ext|Ext|Path|Path with Filename.Ext";

local pattern = { -- pattern, description
    {".",           "all chars"},
    {"%w", "alphanumeric chars"},
    {"%s",             "spaces"},
    {"%a",            "letters"},
    {"%l", "lower case letters"},
    {"%u", "upper case letters"},
    {"%d",             "digits"},
    {"%p",  "punctuation chars"},
    {"%.",                  nil},
    {"%s%-%s",              nil},
}

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex > -1) and (FieldIndex < #pattern) then
        local fieldname = "Chars Stat (";
        if (pattern[FieldIndex+1][2] ~= nil) then
            fieldname = fieldname .. pattern[FieldIndex+1][2] .. ')';
        else
            fieldname = fieldname .. 'pattern "' .. pattern[FieldIndex+1][1] .. '")';
        end
        return fieldname, UnitsStr, 1;
    elseif (FieldIndex == #pattern) then
        return "Lower Case Check", UnitsStr, 6;
    elseif (FieldIndex == #pattern+1) then
        return "Upper Case Check", UnitsStr, 6;
    elseif (FieldIndex == #pattern+2) then
        return "Case Duplicates Check", '', 6;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (DelimPattern == nil) then
        DelimPattern = "/\\";
    end
    local targetpath = FileName;
    if (utf8.find(FileName, "[" .. DelimPattern .. "]%.%.$")) then
        targetpath = utf8.sub(FileName, 1, -3);
    end
    local isDir = SysUtils.DirectoryExists(targetpath);
    local target = getTargetStr(targetpath, isDir, UnitIndex);
    if (FieldIndex > -1) and (FieldIndex < #pattern) then
        return calcChr(target, pattern[FieldIndex+1][1]);
    elseif (FieldIndex == #pattern) then
        return checkCase(target, false);
    elseif (FieldIndex == #pattern+1) then
        return checkCase(target, true);
    elseif (FieldIndex == #pattern+2) then
        return checkCaseDupl(targetpath);
    end
    return nil; -- invalid
end

function getPath(Str)
    if (Str == nil) then
        return nil;
    end
    return utf8.match(Str, "(.*[" .. DelimPattern .. "])");
end

function getFullname(Str)
    if (Str == nil) then
        return nil;
    end
    return utf8.match(Str, "[" .. DelimPattern .. "]([^" .. DelimPattern .. "]+)$");
end

function getFilename(Str, IsDir)
    local FileName = nil;
    local FullName = getFullname(Str);
    if (FullName ~= nil) and (IsDir == false) then
        FileName = utf8.match(FullName, "(.+)%..+");
    end
    if (FileName ~= nil) then
        return FileName;
    else
        return FullName;
    end
end

function getExt(Str, IsDir)
    if (Str == nil) or (IsDir == true) then
        return nil;
    end
    if (getFilename(Str, IsDir) == getFullname(Str)) then
        return nil;
    end
    return utf8.match(Str, ".+%.(.+)$");
end

function getTargetStr(FullPath, IsDir, UnitIndex)
    if (UnitIndex == 0) then
        return getFilename(FullPath, IsDir);
    elseif (UnitIndex == 1) then
        return getFullname(FullPath);
    elseif (UnitIndex == 2) then
        return getExt(FullPath, IsDir);
    elseif (UnitIndex == 3) then
        return utf8.match(FullPath, "(.*[" .. DelimPattern .. "])");
    elseif (UnitIndex == 4) then
        return FullPath;
    end
    return nil;
end

function calcChr(Target, Pattern)
    if (Target ~= nil) and (Pattern ~= nil) then
        local Count = 0;
        for Chr in utf8.gmatch(Target, Pattern) do
            Count = Count + 1;
        end
        return Count;
    end
end

function checkCase(Target, UpperCase)
    if (Target == nil) then
        return nil;
    end
    if (UpperCase == true) then
        if (Target == utf8.upper(Target)) then
            return true;
        end
    elseif (UpperCase == false) then
        if (Target == utf8.lower(Target)) then
            return true;
        end
    end
    return false;
end

function checkCaseDupl(Str)
    local IsFound = false;
    local CurrentPath = getPath(Str);
    local CurrentName = getFullname(Str);
    local Handle, FindData = SysUtils.FindFirst(CurrentPath .. "*");
    if (Handle ~= nil) then
        repeat
            if (FindData.Name ~= CurrentName) and (utf8.lower(CurrentName) == utf8.lower(FindData.Name)) then
                IsFound = true;
                break;
            end
                Result, FindData = SysUtils.FindNext(Handle);
        until (Result == nil)
        SysUtils.FindClose(Handle);
    end
    return IsFound;
end
