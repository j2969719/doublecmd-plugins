-- filenamematchwdx.lua

local pattern = {
    {"(%d+)%.", 1, "(number)."}, -- pattern, type, name 
    {"%-%s+(%d+)%.", 1, " (number)."}, 
    {"^(.-)%s+%-", 8}, 
    {"%-%s+(.+)", 8}, 
    {".+%-%s(.+)$", 8}, 
    {"%-%s+%d+%.(.+)$", 8}, 
    {"%-%s(.-)%s+%-", 8}, 
    {"%.tar%.(.-)$", 8, "tar.(xyz)"}, 
}

function ContentGetSupportedField(Index)
    if (pattern[Index + 1] ~= nil ) then
        if (pattern[Index + 1][3] ~= nil) then
            return pattern[Index + 1][3], "Filename|Filename.Ext|Path|Path with Filename.Ext", pattern[Index + 1][2];
        else
            return pattern[Index + 1][1], "Filename|Filename.Ext|Path|Path with Filename.Ext", pattern[Index + 1][2];
        end
    else
        return '', '', 0; -- ft_nomorefields
    end
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return nil;
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local tname = FileName;
    if (string.find(FileName, "[/\\]%.%.$")) then
        tname = string.sub(FileName, 1, - 3);
    end
    if (UnitIndex == 0) then
        tname = getFilename(tname);
    elseif (UnitIndex == 1) then
        tname = string.match(tname, "^.*[/\\](.+[^/\\])$");
    elseif (UnitIndex == 2) then
        tname = string.match(tname, "(.*[/\\])");
    end
    if (tname ~= nil) then
        return tname:match(pattern[FieldIndex + 1][1]);
    end
    return nil;
end

function getFilename(str)
    local target = string.match(str, "^.*[/\\](.+[^/\\])$");
    if (target ~= nil) then
        if (string.match(target, "(.+)%..+")) then
            target = string.match(target, "(.+)%..+");
        end
    else
        target = nil;
    end
    return target;
end
