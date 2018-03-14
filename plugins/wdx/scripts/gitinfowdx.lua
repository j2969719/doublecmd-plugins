-- gitinfowdx.lua

local fields = {
    {"Subject", 8, "git log -1 --pretty=format:%s"}, 
    {"Body", 8, "git log -1 --pretty=format:%b"}, 
    {"Commit hash", 8, "git log -1 --pretty=format:%H"}, 
    {"Committer name", 8, "git log -1 --pretty=format:%cn"},
    {"Committer email", 8, "git log -1 --pretty=format:%ce"},
    {"Сommitter date", 8, "git log -1 --pretty=format:%cd --date=format:'%d.%m.%Y %H:%M'"}, 
    {"Author name", 8, "git log -1 --pretty=format:%an"}, 
    {"Author email", 8, "git log -1 --pretty=format:%ae"}, 
    {"Author date", 8, "git log -1 --pretty=format:%ad --date=format:'%d.%m.%Y %H:%M'"}, 
    {"Branch", 8, "git rev-parse --abbrev-ref HEAD #"},
    {"Origin URL", 8, "git remote get-url origin #"},
    {"Push URL", 8, "git remote get-url origin --push #"},
    {"Log", 8, "git log --oneline"}
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
    local attr = SysUtils.FileGetAttr(FileName);
    if (attr > 0) then
        if (math.floor(attr / 0x00000004) % 2 ~= 0)  then
            return nil; 
        end
    end
    if (fields[FieldIndex + 1][3] ~= nil) then
        local handle = io.popen(fields[FieldIndex + 1][3] .. ' "'..FileName..'"', 'r');
        local result = handle:read("*a");
        handle:close();
        if (fields[FieldIndex + 1][1]=="Branch") or (fields[FieldIndex + 1][1]=="Origin URL") or (fields[FieldIndex + 1][1]=="Push URL") then
            result = result:sub(1, - 2);
        end
        return result;
    end
    return nil; -- invalid
end
