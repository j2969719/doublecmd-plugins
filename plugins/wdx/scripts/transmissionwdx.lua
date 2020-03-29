-- requires transmission-show

local cmd = "transmission-show"
local params = ''
local output = ''
local filename = ''

local fields = {
    {"Name",                8,         "%s%sName:%s([^\n]+)"},      -- name, field type, pattern
    {"Hash",                8,         "%s%sHash:%s([^\n]+)"}, 
    {"Created by",          8,  "%s%sCreated%sby:%s([^\n]+)"}, 
    {"Created on",          8,  "%s%sCreated%son:%s([^\n]+)"}, 
    {"Comment",             8,      "%s%sComment:%s([^\n]+)"}, 
    {"Piece Count",         2, "%s%sPiece%sCount:%s([^\n]+)"}, 
    {"Piece Size",          8,  "%s%sPiece%sSize:%s([^\n]+)"}, 
    {"Total Size",          8,  "%s%sTotal%sSize:%s([^\n]+)"}, 
    {"Privacy",             8,      "%s%sPrivacy:%s([^\n]+)"}, 
    {"Piece Size (bytes)",  2,  "%s%sPiece%sSize:%s([^\n]+)"}, 
    {"Total Size (bytes)",  2,  "%s%sTotal%sSize:%s([^\n]+)"}, 
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
        return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2];
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
            if (FieldIndex > 8) then
                mult = result:match("%w+$");
                size = result:gsub("[^%d%.]", '');
                size = tonumber(size)
                if (mults[mult] ~= nil) then
                    size = size * mults[mult];  
                end
                result = size;
            elseif (FieldIndex == 3) then
                month, day, dtime, year = result:match("%w+%s+(%w+)%s+(%d+)%s+([%d:]+)%s+(%d+)");
                if (tonumber(day) < 10) then
                    day = '0' .. day
                end
                if (months[month] ~= nil) and (year ~= nil)  and (day ~= nil) and (dtime ~= nil) then
                    result = year .. '-' .. months[month] .. '-' .. day .. ' ' .. dtime;
                end
            end
        end
    end
    return result;
end
