local cmd = "svn info"

local fields = {
    {"Path", 8},
    {"Name", 8},
    {"Working Copy Root Path", 8},
    {"URL", 8},
    {"Relative URL", 8},
    {"Repository Root", 8},
    {"Repository UUID", 8},
    {"Revision", 2},
    {"Node Kind", 8},
    {"Schedule", 8},
    {"Last Changed Author", 8},
    {"Last Changed Rev", 2},
    {"Last Changed Date", 8},
    {"Text Last Updated", 8},
    {"Checksum", 8}
}

local filename = ""
local res = nil

function ContentGetSupportedField(Index)
    if (fields[Index + 1] ~= nil ) then
        return fields[Index + 1][1], "", fields[Index + 1][2];
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
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) then
            return nil;
        end        
        local handle = io.popen(cmd .. ' "' .. FileName .. '"', 'r');
        res = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    local tmp = string.match(res, fields[FieldIndex + 1][1] .. ":%s[^\n]+");
    if tmp ~= nil then
        return tmp:gsub("^" .. fields[FieldIndex + 1][1] .. ":%s", "");
    end
    return nil; -- invalid
end
