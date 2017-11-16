-- filenamematchwdx.lua

local pattern = {
    {"(%d+)%.", 1},
    {"%-%s+(%d+)%.", 1},
    {"(.+)%s+%-", 8},
    {"%-%s+(.+)", 8},
}

function ContentGetSupportedField(Index)
    if (pattern[Index + 1] ~= nil ) then
        return pattern[Index + 1][1], "", pattern[Index + 1][2];
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
    local target = '';
    local tname = FileName;
    if (string.find(FileName, "[/\\]%.%.$")) then
        tname = string.sub(FileName, 1, - 3);
    end
    target = tname:match("^.*[/\\](.+[^/\\])$");
    if (target ~= nil) then
        if (string.match(target, "(.+)%..+")) then
            target = string.match(target, "(.+)%..+");
        end
    else
        target = '';
    end
    return target:match(pattern[FieldIndex + 1][1]);
end
