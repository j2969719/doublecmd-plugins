-- filenamechrstatwdx.lua
-- luarocks-5.1 install luautf8
-- the search for duplicate file names is a bit unstable
 
local utf8 = require("lua-utf8")

local pattern = {
    {".", "all chars"}, 
    {"%w", "alphanumeric chars"}, 
    {"%s", "spaces"}, 
    {"%a", "letters"}, 
    {"%l", "lower case letters"}, 
    {"%u", "upper case letters"}, 
    {"%d", "digits"}, 
    {"%p", "punctuation chars"}, 
    {"%."}, 
    {"%s%-%s"}, 
}
local fnames = {}

function ContentGetSupportedField(Index)
    local submi = '';
    for i = 1, #pattern do
        if (i > 1) then
            submi = submi .. '|';
        end
        if (pattern[i][2] ~= nil) then
            submi = submi .. pattern[i][2];
        else
            submi = submi .. 'pattern "' .. pattern[i][1].. '"';
        end
    end
    if (Index == 0) then
        return 'Filename', submi, 1;
    elseif (Index == 1) then
        return 'Filename.Ext', submi, 1;
    elseif (Index == 2) then
        return 'Ext', submi, 1;
    elseif (Index == 3) then
        return 'Path', submi, 1;
    elseif (Index == 4) then
        return 'Path with Filename.Ext', submi, 1;
    elseif (Index == 5) then
        return 'lower case', 'Filename|Filename.Ext|Ext', 6;
    elseif (Index == 6) then
        return 'upper case', 'Filename|Filename.Ext|Ext', 6;
    elseif (Index == 7) then
        return '(case) duplicate filename', '', 6;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return nil;
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local target;
    local tname = FileName;
    if (utf8.find(FileName, "[/\\]%.%.$")) then
        tname = utf8.sub(FileName, 1, - 3);
    end
    if (FieldIndex  == 0) then
        target = getFilename(tname)
    elseif (FieldIndex == 1) then
        target = getFullname(tname)
    elseif (FieldIndex == 2) then
        target = getExt(tname)
    elseif (FieldIndex == 3) then
        target = utf8.match(tname, "(.*[/\\])");
    elseif (FieldIndex == 4) then
        target = tname; 
    elseif (FieldIndex == 5) then 
        if (UnitIndex == 0) and (getFilename(tname) == utf8.lower(getFilename(tname))) then
            return  true;
        elseif (UnitIndex == 1) and (getFullname(tname) == utf8.lower(getFullname(tname))) then
            return  true; 
        elseif (UnitIndex == 2) and (getExt(tname) == utf8.lower(getExt(tname))) then
            return  true; 
        end 
        return false; 
    elseif (FieldIndex == 6) then 
        if (UnitIndex == 0) and (getFilename(tname) == utf8.upper(getFilename(tname))) then
            return  true;
        elseif (UnitIndex == 1) and (getFullname(tname) == utf8.upper(getFullname(tname))) then
            return  true; 
        elseif (UnitIndex == 2) and (getExt(tname) == utf8.upper(getExt(tname))) then
            return  true; 
        end 
        return false;           
    elseif (FieldIndex == 7) then         
        local fpath = utf8.match(tname, "(.*[/\\])");
        local fname = getFullname(tname);
        if (#fnames ~= 0) then
            for i = 1, #fnames do
                local tpath = utf8.match(fnames[i], "(.*[/\\])");
                local tfname = getFullname(fnames[i]);
                if (fpath == tpath) and (fname ~= tfname) and (utf8.lower(fname) == utf8.lower(tfname)) then
                    return true;
                end
            end
            fnames[#fnames + 1] = tname;
            return false;
        else
            fnames[1] = tname;
            return false;        
        end
    end
    if (target ~= nil) and (pattern[UnitIndex + 1][1] ~= nil) then
        local count = 0;
        for char in utf8.gmatch(target, pattern[UnitIndex + 1][1]) do 
            count = count + 1; 
        end
        return count;
    end 
    return nil; -- invalid
end

function getFilename(str)
    local target = utf8.match(str, "^.*[/\\](.+[^/\\])$");
    if (target ~= nil) then
        if (utf8.match(target, "(.+)%..+")) then
            target = utf8.match(target, "(.+)%..+");
        end
    else
        target = nil;
    end
    return target;
end

function getFullname(str)
    return utf8.match(str, "^.*[/\\](.+[^/\\])$");
end

function getExt(str)
    return utf8.match(str, ".+%.(.+)$");
end
