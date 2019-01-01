-- gitinfowdx.lua

local result_true = "☑️"
local result_false = "-"

local fields = {  -- field name, field type, command, strip newline, sort order
    {"Subject",         8,                                 "git log -1 --pretty=format:%s", false,  1},
    {"Body",            8,                                 "git log -1 --pretty=format:%b", false,  1},
    {"Commit hash",     8,                                 "git log -1 --pretty=format:%H", false,  1},
    {"Committer name",  8,                                "git log -1 --pretty=format:%cn", false,  1},
    {"Committer email", 8,                                "git log -1 --pretty=format:%ce", false,  1},
    {"Сommitter date",  8, "git log -1 --pretty=format:%cd --date=format:'%d.%m.%Y %H:%M'", false,  1},
    {"Author name",     8,                                "git log -1 --pretty=format:%an", false,  1},
    {"Author email",    8,                                "git log -1 --pretty=format:%ae", false,  1},
    {"Author date",     8, "git log -1 --pretty=format:%ad --date=format:'%d.%m.%Y %H:%M'", false,  1},
    {"Branch",          8,                             "git rev-parse --abbrev-ref HEAD #", true,   1},
    {"Revision",        2,                                   "git rev-list --count HEAD #", true,   1},
    {"Origin URL",      8,                                   "git remote get-url origin #", true,   1},
    {"Push URL",        8,                            "git remote get-url origin --push #", true,   1},
    {"Log",             9,                                             "git log --oneline", false,  1},
    {"Untracked",       6,                                               "git ls-files -o", false,  1},
    {"Ignored",         6,                            "git ls-files -i --exclude-standard", true,  -1},
    {"Modified",        6,                                               "git ls-files -m", true,  -1},
    {"Untracked (alt)", 7,                                               "git ls-files -o", true,  -1},
    {"Ignored (alt)",   7,                            "git ls-files -i --exclude-standard", true,  -1},
    {"Modified (alt)",  7,                                               "git ls-files -m", true,  -1},
}

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        return fields[FieldIndex + 1][1], '', fields[FieldIndex + 1][2];
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
    return fields[FieldIndex + 1][5];
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
    local attr = SysUtils.FileGetAttr(FileName);
    if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) then
        return nil;
    end
    local dir = FileName:match("(.*[" .. delimpat .. "])");
    if (fields[FieldIndex + 1][3] ~= nil) then
        local handle = io.popen('cd "'..dir..'" && ' .. fields[FieldIndex + 1][3] .. ' "'..FileName..'"', 'r');
        local result = handle:read("*a");
        handle:close();
        if (result ~= nil) then
            if (fields[FieldIndex + 1][4] == true) then
                result = result:sub(1, - 2);
            end
            if (fields[FieldIndex + 1][2] == 6) then
                if (result ~= '') then
                    return true;
                else
                    return false;
                end
            elseif (fields[FieldIndex + 1][2] == 7) then
                if (result ~= '') then
                    return result_true;
                else
                    return result_false;
                end
            elseif (fields[FieldIndex + 1][2] == 2) then
                return tonumber(result);
            elseif (fields[FieldIndex + 1][2] == 9) then
                if (UnitIndex == 0) then
                    return result;
                else
                    return nil;
                end
            else
                return result;
            end
        end
    end
    return nil; -- invalid
end
