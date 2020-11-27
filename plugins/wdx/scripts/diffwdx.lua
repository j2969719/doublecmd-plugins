
local cmds = {
    {"git diff", "git diff"},
    {"git diff (1 day ago)", "git diff 'master@{1 day ago}..master'"},
    {"git diff (3 day ago)", "git diff 'master@{3 day ago}..master'"},
    {"git diff (5 day ago)", "git diff 'master@{5 day ago}..master'"},
    {"git diff (7 day ago)", "git diff 'master@{7 day ago}..master'"},
    {"svn diff", "svn diff"},
}

local files = {
    "/home/user/somefile",
    "/home/user/somefile1",
    "/home/user/somefile2",
}

function ContentGetSupportedField(FieldIndex)
    if (cmds[FieldIndex + 1] ~= nil) then
        return cmds[FieldIndex + 1][1], '', 9;
    elseif (files[FieldIndex - #cmds + 1] ~= nil) then
        return 'diff to the ' .. files[FieldIndex - #cmds  + 1], '', 6;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if not SysUtils.DirectoryExists(FileName) and (UnitIndex == 0) then
        if (cmds[FieldIndex + 1] ~= nil) then
            return getOutput (cmds[FieldIndex + 1][2] .. ' "' .. FileName:gsub('"', '\\"') .. '"');
        elseif (files[FieldIndex - #cmds  + 1] ~= nil) then
            if (getOutput('diff -q "' .. FileName:gsub('"', '\\"') .. '"' .. ' "' .. files[FieldIndex - #cmds  + 1] .. '"') ~= '') then
                return true;
            else
                return false;
            end
        end
    end
    return nil; -- invalid
end

function getOutput(command)
    local handle = io.popen(command, 'r');
    local output = handle:read("*a");
    handle:close();
    return output;
end
