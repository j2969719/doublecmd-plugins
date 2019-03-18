local datefmt = {
    "DD.MM.YYYY hh:mm", 
    "DD.MM.YYYY hh:mm:ss", 
    "DD.MM.YY hh:mm", 
    "DD-MM-YYYY hh:mm", 
    "MM-DD-YYYY hh:mm", 
    "YYYY-MM-DD hh:mm", 
    "DD/MM/YYYY hh:mm", 
    "DD.MM.YYYY", 
}

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'FileName', '', 8; -- FieldName,Units,ft_string
    elseif (FieldIndex == 1) then
        return 'Path', '', 8;
    elseif (FieldIndex == 2) then
        local units = datefmt[1];
        for i = 2, #datefmt do
            units = units .. '|' .. datefmt[i]; 
        end 
        return 'Deletion Date', units, 8;
    elseif (FieldIndex == 3) then
        return 'Deletion Date (number)', "YYYY|YY|MM|DD|hh|mm|ss", 1;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if FileName:find("/%.?Trash%-?%d*/files/.-$") then
        local result;
        local year, mounth, day, hour, minutes, seconds;
        local basename = FileName:match("[^/]+$");
        if not SysUtils.DirectoryExists(FileName) then
            fname = basename:match("(.+)%..+"); 
            if (fname == nil) then
                fname = basename;
            end
        end
        local fname = FileName:match("[^/]+$");
        local trashinfo = FileName:match("(.+)files/.-$") .. "info/" .. fname .. ".trashinfo";
        local file = io.open(trashinfo, 'r');
        if (file ~= nil) then
            for line in file:lines() do 
                if (FieldIndex < 2) and line:find("^Path=") then
                    result = line:gsub("^Path=", "");
                    break;
                elseif (FieldIndex >= 2) and line:find("^DeletionDate=") then
                    result = line:gsub("^DeletionDate=", "");
                    break;
                end
            end
            file:close();
            if (result ~= nil) and (FieldIndex < 2) then
                result = result:gsub("%%(..)", function(c) return string.char(tonumber(c, 16)) end);
            end
            if (result ~= nil) and (FieldIndex >= 2) then
                year, mounth, day, hour, minutes, seconds = result:match("(....)%-(..)%-(..)T(..):(..):(..)");
                year_short = year:match("..$");
                if  (FieldIndex == 2) then
                    result = datefmt[UnitIndex + 1];
                    result = result:gsub("YYYY",     year);
                    result = result:gsub("YY", year_short);
                    result = result:gsub("MM",     mounth);
                    result = result:gsub("DD",        day);
                    result = result:gsub("hh",       hour);
                    result = result:gsub("mm",    minutes);
                    result = result:gsub("ss",    seconds);
                end
            end
            if (FieldIndex == 0) and (result ~= nil) then
                return result:match("[^/]+$");
            elseif (FieldIndex == 1) then
                return result;
            elseif (FieldIndex == 2) then
                return result;
            elseif (FieldIndex == 3) and (result ~= nil) then
                if (UnitIndex == 0) then
                    return tonumber(year);
                elseif (UnitIndex == 1) then
                    return tonumber(year_short);
                elseif (UnitIndex == 2) then
                    return tonumber(mounth);
                elseif (UnitIndex == 3) then
                    return tonumber(day);
                elseif (UnitIndex == 4) then
                    return tonumber(hour);
                elseif (UnitIndex == 5) then
                    return tonumber(minutes);
                elseif (UnitIndex == 6) then
                    return tonumber(seconds);
                end
            end
        end
    end 
    return nil; -- invalid
end