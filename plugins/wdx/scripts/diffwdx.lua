
local files = {
    "/home/user/somefile", 
    "/home/user/somefile1", 
    "/home/user/somefile2", 
}

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'git diff', '', 9;
    elseif (FieldIndex == 1) then
        return 'svn diff', '', 9;
    elseif (files[FieldIndex - 1] ~= nil) then
        return 'diff to the ' .. files[FieldIndex - 1], '', 6;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if not SysUtils.DirectoryExists(FileName) and (UnitIndex == 0) then
        if (FieldIndex == 0) then
            return getOutput('git diff "' .. FileName .. '"');
        elseif (FieldIndex == 1) then
            return getOutput('svn diff "' .. FileName .. '"');
        elseif (files[FieldIndex - 1] ~= nil) then
            if (getOutput('diff -q "' .. FileName .. '"' .. ' "' .. files[FieldIndex - 1] .. '"') ~= '') then
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
