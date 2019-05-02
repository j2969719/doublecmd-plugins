
local ext_allowed = {
    "mp3", "flac", "ogg", "wav", 
    "pdf", "doc", "docx", "odt", 
}

local extlist = {}
local lastdir = ''
local units = ''
local bytes = 1024 -- or 1000

function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
    for i = 1, #ext_allowed do
        if (units == '') then
            units = ext_allowed[i];
        else
            units = units .. '|' .. ext_allowed[i];
        end
    end
end

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return "count", units, 2;
    elseif (FieldIndex == 1) then
        return "size", units, 2;
    elseif (FieldIndex == 2) then
        return "size (K)", units, 3;
    elseif (FieldIndex == 3) then
        return "size (M)", units, 3;
    elseif (FieldIndex == 4) then
        return "size (G)", units, 3;
    elseif (FieldIndex == 5) then
        return "size (T)", units, 3;
    elseif (FieldIndex == 6) then
        return "tooltip", "default|recursively", 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if SysUtils.DirectoryExists(FileName) then
        if (lastdir ~= FileName) then
            extlist = {};
            if (FieldIndex == 6) and (UnitIndex == 1) then
                chkdir(FileName, true);
            else
                chkdir(FileName, false);
            end
            lastdir = FileName;
        end
        if (FieldIndex == 6) then
            local result = '';
            for ext, val in pairs(extlist) do
                if string.find('|' .. units .. '|', '|' .. ext .. '|') then
                    if (result == '') then
                        result = ext .. ": " .. val["count"] .. ', ' .. strsize(val["size"]);
                    else
                        result = result .. '\n' .. ext .. ": " .. val["count"] .. ', ' .. strsize(val["size"]);
                    end
                end
            end
            return result;
        else
            local ext = ext_allowed[UnitIndex + 1];
            if (FieldIndex == 0) and (extlist[ext] ~= nil) then
                return extlist[ext]["count"];
            elseif (FieldIndex == 1) and (extlist[ext] ~= nil) then
                return extlist[ext]["size"];
            elseif (extlist[ext] ~= nil) then
                return extlist[ext]["size"] / math.pow(bytes, FieldIndex - 1);
            end
        end
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
    if (size > math.pow(bytes, 4)) then
        return string.format("%.1f T", size / math.pow(bytes, 4));
    elseif (size > math.pow(bytes, 3)) then
        return string.format("%.1f G", size / math.pow(bytes, 3));
    elseif (size > math.pow(bytes, 2)) then
        return string.format("%.1f M", size / math.pow(bytes, 2));
    elseif (size > bytes) then
        return string.format("%.1f K", size / bytes);
    end
    return size;
end