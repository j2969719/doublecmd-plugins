local cmd = "svn info"

local fields = {
    {"Path",                   8,                        "Path:%s([^\n]+)"},  -- name, field type, pattern
    {"Name",                   8,                      "\nName:%s([^\n]+)"},
    {"Working Copy Root Path", 8, "\nWorking%sCopy%sRoot%sPath:%s([^\n]+)"},
    {"URL",                    8,                       "\nURL:%s([^\n]+)"},
    {"Relative URL",           8,             "\nRelative%sURL:%s([^\n]+)"},
    {"Repository Root",        8,          "\nRepository%sRoot:%s([^\n]+)"},
    {"Repository UUID",        8,          "\nRepository%sUUID:%s([^\n]+)"},
    {"Revision",               2,                  "\nRevision:%s([^\n]+)"},
    {"Node Kind",              8,                "\nNode%sKind:%s([^\n]+)"},
    {"Schedule",               8,                  "\nSchedule:%s([^\n]+)"},
    {"Last Changed Author",    8,     "\nLast%sChanged%sAuthor:%s([^\n]+)"},
    {"Last Changed Rev",       2,        "\nLast%sChanged%sRev:%s([^\n]+)"},
    {"Last Changed Date",      8,       "\nLast%sChanged%sDate:%s([^\n]+)"},
    {"Text Last Updated",      8,       "\nText%sLast%sUpdated:%s([^\n]+)"},
    {"Checksum",               8,                 "\nRChecksum:%s([^\n]+)"},
}

local filename = ''
local output = ''

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2];
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
    local delimpat = SysUtils.PathDelim;
    if (delimpat == nil) then
        delimpat = "/\\";
    end
    if (FileName:find("[" .. delimpat .. "]%.%.$")) then
        return nil;
    end
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) then
            return nil;
        end
        local handle = io.popen(cmd .. ' "' .. FileName:gsub('"', '\\"') .. '"', 'r');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    if (output ~= nil) and (fields[FieldIndex + 1][3] ~= nil) then
        return output:match(fields[FieldIndex + 1][3]);
    end
    return nil; -- invalid
end
