
local ext_allowed = { -- add extensions here
    "mp3", "flac", "ogg", "wav",
    "pdf", "doc", "docx", "odt",

}

local notfound = "no suitable filetypes found"
local bytes = 1024 -- or 1000


local extlist = {}
local lastdir = ''
local lastmode = nil

local units = ''
for i = 1, #ext_allowed do
    if (units == '') then
        units = ext_allowed[i];
    else
        units = units .. '|' .. ext_allowed[i];
    end
end

local fields = {
    {"count",                 2, units, false, 0},
    {"count (recursively)",   2, units,  true, 0},
    {"size",                  2, units, false, 0},
    {"size (recursively)",    2, units,  true, 0},
    {"size K",                3, units, false, 1},
    {"size K (recursively)",  3, units,  true, 1},
    {"size M",                3, units, false, 2},
    {"size M (recursively)",  3, units,  true, 2},
    {"size G",                3, units, false, 3},
    {"size G (recursively)",  3, units,  true, 3},
    {"size T",                3, units, false, 4},
    {"size T (recursively)",  3, units,  true, 4},
    {"tooltip",               8, '',    false, 0},
    {"tooltip (recursively)", 8, '',     true, 0},
}

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil) then
        return fields[FieldIndex + 1][1], fields[FieldIndex + 1][3], fields[FieldIndex + 1][2];
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if SysUtils.DirectoryExists(FileName) then
        if (lastdir ~= FileName) or (lastmode ~= fields[FieldIndex + 1][4]) then
            extlist = {};
            if (fields[FieldIndex + 1][4] == true) then
                chkdir(FileName, true);
            else
                chkdir(FileName, false);
            end
            lastdir = FileName;
            lastmode = fields[FieldIndex + 1][4];
        end
        local result = nil;
        if (fields[FieldIndex + 1][2] == 8) then
            result = '';
            for ext, val in pairs(extlist) do
                if string.find('|' .. units .. '|', '|' .. ext .. '|') then
                    if (result == '') then
                        result = ext .. ": " .. val["count"] .. ', ' .. strsize(val["size"]);
                    else
                        result = result .. '\n' .. ext .. ": " .. val["count"] .. ', ' .. strsize(val["size"]);
                    end
                end
            end
            if (result == '') then
                result = notfound;
            end
        else
            local ext = ext_allowed[UnitIndex + 1];
            if (extlist[ext] ~= nil) then
                if (FieldIndex < 2) then
                    result = extlist[ext]["count"];
                else
                    result = extlist[ext]["size"];
                end
                if (fields[FieldIndex + 1][5] > 0) then
                    result = result / math.pow(bytes, fields[FieldIndex + 1][5]);
                end
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