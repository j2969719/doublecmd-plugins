
local extlist = {}
local lastdir = ''

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'ext stat', 'default|recursively', 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if SysUtils.DirectoryExists(FileName) then
        if (lastdir ~= FileName) then
            extlist = {};
            if (UnitIndex == 1) then
                chkdir(FileName, true);
            else
                chkdir(FileName, false);
            end
            lastdir = FileName;
        end
        local result = '';
        for ext, val in pairs(extlist) do
            if (result == '') then
                result = ext .. ": " .. val["count"] .. ', = ' .. strsize(val["size"]);
            else
                result = result .. '\n' .. ext .. ": " .. val["count"] .. ', = ' .. strsize(val["size"]);
            end
        end 
        return result; 
    end
    return nil;
end

function chkdir(path, recursive)
    local handle, FindData = SysUtils.FindFirst(path .. SysUtils.PathDelim .. '*');
    if (handle ~= nil) then
        repeat
            if (FindData.Name ~= ".") and (FindData.Name ~= "..") then
                if (math.floor(FindData.Attr / 0x00000010) % 2 ~= 0) and (recursive == true) then
                    chkdir(path .. SysUtils.PathDelim .. FindData.Name, recursive);
                else
                    local ext = FindData.Name:match(".+%.(.+)$");
                    if (ext ~= nil) then
                        ext = ext:lower();
                        if (extlist[ext] ~= nil) then
                            extlist[ext]["count"] = extlist[ext]["count"] + 1;
                            extlist[ext]["size"] = extlist[ext]["size"] + FindData.Size;
                        else
                            extlist[ext] = {};
                            extlist[ext]["count"] = 1;
                            extlist[ext]["size"] = FindData.Size;
                        end
                    end
                end
            end
            found, FindData = SysUtils.FindNext(handle);
        until (found == nil)
        SysUtils.FindClose(handle);
    end
end

function strsize(size)
    if (size > math.pow(1024, 4)) then
        return string.format("%.1f T", size / math.pow(1024, 4));
    elseif (size > math.pow(1024, 3)) then
        return string.format("%.1f G", size / math.pow(1024, 3));
    elseif (size > math.pow(1024, 2)) then
        return string.format("%.1f M", size / math.pow(1024, 2));
    elseif (size > 1024) then
        return string.format("%.1f K", size / 1024);
    end
    return size;
end