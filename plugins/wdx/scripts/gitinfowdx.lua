-- gitinfowdx.lua

local fields = {  -- field name, field type, command
    {"Subject",         8,                                 "git log -1 --pretty=format:%s"},
    {"Body",            8,                                 "git log -1 --pretty=format:%b"},
    {"Commit hash",     8,                                 "git log -1 --pretty=format:%H"},
    {"Committer name",  8,                                "git log -1 --pretty=format:%cn"},
    {"Committer email", 8,                                "git log -1 --pretty=format:%ce"},
    {"Сommitter date",  8, "git log -1 --pretty=format:%cd --date=format:'%d.%m.%Y %H:%M'"},
    {"Author name",     8,                                "git log -1 --pretty=format:%an"},
    {"Author email",    8,                                "git log -1 --pretty=format:%ae"},
    {"Author date",     8, "git log -1 --pretty=format:%ad --date=format:'%d.%m.%Y %H:%M'"},
    {"Branch",          8,                             "git rev-parse --abbrev-ref HEAD #"},
    {"Origin URL",      8,                                   "git remote get-url origin #"},
    {"Push URL",        8,                            "git remote get-url origin --push #"},
    {"Log",             8,                                             "git log --oneline"},
    {"Untracked",       6,                                               "git ls-files -o"},
    {"Ignored",         6,                            "git ls-files -i --exclude-standard"},
    {"Modified",        6,                                               "git ls-files -m"},
}

local stripnewline = {  -- strip trailing linebreak from output
    {"Branch"},  -- field name
    {"Origin URL"},
    {"Push URL"},
    {"Untracked"},
    {"Ignored"},
    {"Modified"},
}

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
    if (fields[FieldIndex + 1][3] ~= nil) then
        local handle = io.popen(fields[FieldIndex + 1][3] .. ' "'..FileName..'"', 'r');
        local result = handle:read("*a");
        handle:close();
        if (result ~= nil) then
            for i = 1, #stripnewline do
                if (fields[FieldIndex + 1][1] == stripnewline[i][1]) then
                    result = result:sub(1, -2);
                    break;
                end
            end
            if (fields[FieldIndex + 1][2] == 6) then
                if (result ~= "") then
                    return true;
                else
                    return false;
                end
            else
                return result;
            end
        end
    end
    return nil; -- invalid
end
