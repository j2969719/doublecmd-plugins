-- requires transmission-show

local cmd = "transmission-show"
local params = ''
local output = ''
local filename = ''

local fields = {
    {"Name",        8,         "%s%sName:%s([^\n]+)"},  -- name, field type, pattern
    {"Hash",        8,         "%s%sHash:%s([^\n]+)"}, 
    {"Created by",  8,  "%s%sCreated%sby:%s([^\n]+)"}, 
    {"Created on",  8,  "%s%sCreated%son:%s([^\n]+)"}, 
    {"Comment",     8,      "%s%sComment:%s([^\n]+)"}, 
    {"Piece Count", 2, "%s%sPiece%sCount:%s([^\n]+)"}, 
    {"Piece Size",  8,  "%s%sPiece%sSize:%s([^\n]+)"}, 
    {"Total Size",  8,  "%s%sTotal%sSize:%s([^\n]+)"}, 
    {"Privacy",     8,      "%s%sPrivacy:%s([^\n]+)"}, 
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
    if (output ~= nil) and (fields[FieldIndex + 1][3] ~= nil) then
        return output:match(fields[FieldIndex + 1][3]);
    end 
    return nil;
end
