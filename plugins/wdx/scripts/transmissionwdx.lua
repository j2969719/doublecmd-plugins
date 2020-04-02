-- requires transmission-show

local cmd = "transmission-show"
local params = ''
local output = ''
local filename = ''

local fields = {
    {"Name",                8,                     "%s%sName:%s([^\n]+)"},  -- name, field type, pattern
    {"Hash",                8,                     "%s%sHash:%s([^\n]+)"}, 
    {"Created by",          8,              "%s%sCreated%sby:%s([^\n]+)"}, 
    {"Created on",          8,              "%s%sCreated%son:%s([^\n]+)"}, 
    {"Created on (date)",  10,              "%s%sCreated%son:%s([^\n]+)"}, 
    {"Comment",             8,                  "%s%sComment:%s([^\n]+)"}, 
    {"Piece Count",         2,             "%s%sPiece%sCount:%s([^\n]+)"}, 
    {"Piece Size",          8,              "%s%sPiece%sSize:%s([^\n]+)"}, 
    {"Piece Size (bytes)",  3,              "%s%sPiece%sSize:%s([^\n]+)"}, 
    {"Total Size",          8,              "%s%sTotal%sSize:%s([^\n]+)"}, 
    {"Total Size (bytes)",  3,              "%s%sTotal%sSize:%s([^\n]+)"}, 
    {"Privacy",             8,                  "%s%sPrivacy:%s([^\n]+)"}, 
    {"Trackes",             9, "\nTRACKERS\n\n%s%s(.-)\n[WBDFILES]+\n\n"}, 
    {"Webseeds",            9,       "\nWEBSEEDS\n\n%s%s(.+)\nFILES\n\n"}, 
    {"Files",               9,                  "\nFILES\n\n%s%s(.+)\n$"}, 
}

local mults = {
    ["kB"]  =  math.pow(10, 3), 
    ["KB"]  =  math.pow(10, 3), 
    ["MB"]  =  math.pow(10, 6), 
    ["GB"]  =  math.pow(10, 9), 
    ["TB"]  = math.pow(10, 12), 
    ["KiB"] =  math.pow(2, 10), 
    ["MiB"] =  math.pow(2, 20), 
    ["GiB"] =  math.pow(2, 30), 
    ["TiB"] =  math.pow(2, 40), 
}

local months = {
    ["Jan"] = "01", 
    ["Feb"] = "02", 
    ["Mar"] = "03", 
    ["Apr"] = "04", 
    ["May"] = "05", 
    ["Jun"] = "06", 
    ["Jul"] = "07", 
    ["Aug"] = "08", 
    ["Sep"] = "09", 
    ["Oct"] = "10", 
    ["Nov"] = "11", 
    ["Dec"] = "12", 
}

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil) then
        if (fields[FieldIndex + 1][1]:find("(bytes)") ~= nil) then
            return fields[FieldIndex + 1][1], "bytes|K|M|G|T", fields[FieldIndex + 1][2];
        else
            return fields[FieldIndex + 1][1], '', fields[FieldIndex + 1][2];
        end
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return 'EXT="torrent"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            return nil;
        end
        local handle = io.popen(cmd .. ' ' .. params .. ' "' .. FileName .. '"');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    local result = nil;
    if (output ~= nil) and (fields[FieldIndex + 1][3] ~= nil) then
        result = output:match(fields[FieldIndex + 1][3]);
        if (result ~= nil) then
            if (fields[FieldIndex + 1][1]:find("(bytes)") ~= nil) then
                local mult = result:match("%w+$");
                local sizestr = result:gsub("[^%d%.]", '');
                if (sizestr == nil) then
                    return nil;
                end
                local size = tonumber(sizestr);
                if (size == nil) then
                    sizestr = sizestr:gsub("%.", ',');
                    size = tonumber(sizestr);
                    if (size == nil) then
                        return nil;
                    end
                end
                if (mult == nil) then
                    result = size;
                elseif (mults[mult] ~= nil) then
                    size = size * mults[mult];
                    result = size;
                else
                    result = nil;
                end
                if (UnitIndex > 0) and (result ~= nil) then
                    if (UnitIndex == 1) then
                        result = size / mults["KiB"];
                    elseif (UnitIndex == 2) then
                        result = size / mults["MiB"];
                    elseif (UnitIndex == 3) then
                        result = size / mults["GiB"];
                    elseif (UnitIndex == 4) then
                        result = size / mults["TiB"];
                    end
                    result = math.floor(result * 10 + 0.5) / 10;
                end
            elseif (fields[FieldIndex + 1][1]:find("Created on") ~= nil) then
                local month, day, dtime, year = result:match("%w+%s+(%w+)%s+(%d+)%s+([%d:]+)%s+(%d+)");
                if (months[month] ~= nil) and (year ~= nil)  and (day ~= nil) and (dtime ~= nil) then
                    if (fields[FieldIndex + 1][2] ~= 10) then
                        if (tonumber(day) < 10) then
                            day = '0' .. day;
                        end
                        result = year .. '.' .. months[month] .. '.' .. day .. ' ' .. dtime;
                    else
                        local dhour, dmin, dsec = dtime:match("(%d+):(%d+):(%d+)");
                        local unixtime = os.time{year = tonumber(year), month = tonumber(months[month]), 
                                         day = tonumber(day), hour = tonumber(dhour), min = tonumber(dmin), sec = tonumber(dsec)};
                        result = unixtime * 10000000 + 116444736000000000;
                    end
                elseif (fields[FieldIndex + 1][2] == 10) then
                    return nil;
                end
            elseif (fields[FieldIndex + 1][2] == 9) and (UnitIndex ~= 0) then
                return nil;
            end
        end
    end
    return result;
end
