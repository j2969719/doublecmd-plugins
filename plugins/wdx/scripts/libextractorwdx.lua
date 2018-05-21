-- libextractorwdx.lua

local cmd = "extract"

local fields = {
    {"title", 8}, 
    {"author name", 8}, 
    {"creator", 8}, 
    {"format", 8}, 
    {"last saved by", 8}, 
    {"created by software", 8},  
    {"produced by software", 8},  
    {"encoder version", 8}, 
    {"image dimensions", 8}, 
    {"comment", 8}, 
    {"language", 8}, 
    {"page count", 2}, 
    {"word count", 2}, 
    {"line count", 2}, 
    {"character count", 2}, 
    {"paragraph count", 2}, 
    {"editing cycles", 2}, 
    {"creation date", 8}, 
    {"modification date", 8},  
    {"template", 8},  
}

local filename = ""
local res = nil

function ContentGetSupportedField(Index)
    if (fields[Index + 1] ~= nil ) then
        return fields[Index + 1][1], "", fields[Index + 1][2];
    else
        return '', '', 0; -- ft_nomorefields
    end
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            return nil;
        end
        local handle = io.popen(cmd .. ' "' .. FileName .. '"', 'r');
        res = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    return getval(fields[FieldIndex + 1][1]);
end

function getval(str)
    local tmp = string.match(res, "\n" .. str .. "%s%-%s[^\n]+");
    if tmp ~= nil then
        return tmp:gsub("^\n" .. str .. "%s%-%s", "");
    end
    return nil;
end