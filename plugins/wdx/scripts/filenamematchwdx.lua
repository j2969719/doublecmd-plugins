-- filenamematchwdx.lua

-- file groups 
local groups = {
    {"Music", "%.mp3$", "%.ogg$", "%.wma$", "%.wav$"},      -- group name, pattern, ...
    {"Archive", "%.zip$", "%.rar$", "%.7z$", "%.tar%..+$"}, 
    {"Docs", "%.doc$", "%.docx$", "%.rtf$"}, 
}


local pattern = {
    {"(%d+)%.", 1, "(number)."},      -- pattern, type, name, alt pattern, ...
    {"%-%s+(%d+)%.", 1}, 
    {"^(.-)%s+%-", 8, "(part) -"}, 
    {"%-%s+(.+)", 8, "- (part)", "%-%s+%d+%.(.+)$", ".+%-%s(.+)$"}, 
    {"%-%s(.-)%s+%-", 8}, 
    {"%.tar%.(.-)$", 8, "tar.(xyz)"}, 
}

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'Group', '', 8; -- FieldName,Units,ft_string
    elseif (pattern[Index] ~= nil ) then
        if (pattern[Index][3] ~= nil) then
            return pattern[Index][3], "Filename|Filename.Ext|Path|Path with Filename.Ext", pattern[Index][2];
        else
            return "Pattern: " .. pattern[Index][1], "Filename|Filename.Ext|Path|Path with Filename.Ext", pattern[Index][2];
        end
    else
        return '', '', 0; -- ft_nomorefields
    end
end

function ContentGetDetectString()
    return nil;
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local tname = FileName;
    if (string.find(FileName, "[/\\]%.%.$")) then
        tname = string.sub(FileName, 1, - 3);
    end
    if (FieldIndex == 0) then
        if (#groups >= 1) then
            for group = 1, #groups do
                if (#groups[group] > 1) then
                    for i = 2, #groups[group] do
                        if (string.find(tname, groups[group][i])) then
                            return groups[group][1];
                        end
                    end
                end
            end
        end
        return nil;
    end
    if (UnitIndex == 0) then
        tname = getFilename(tname);
    elseif (UnitIndex == 1) then
        tname = string.match(tname, "^.*[/\\](.+[^/\\])$");
    elseif (UnitIndex == 2) then
        tname = string.match(tname, "(.*[/\\])");
    end
    if (tname ~= nil) then
        if (#pattern[FieldIndex] > 3) then
            for i = 4, #pattern[FieldIndex] do
                if (string.find(tname, pattern[FieldIndex][i])) then
                    return tname:match(pattern[FieldIndex][i]);
                end
            end
        end
        return tname:match(pattern[FieldIndex][1]);
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
