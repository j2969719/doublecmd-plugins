
local ft_string = 8
local ft_number = 2
local ft_float  = 3
local ft_bool   = 6

local fields = {
    {"test", "text (%d+)", ft_number},
}

local commands = {
    ['doc'] = 'catdoc -w "$FILE"',
    ['pdf'] = 'pdftotext -layout -nopgbrk "$FILE" -',
    ['odt'] = 'odt2txt "$FILE"',
}

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil) then
        return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][3];
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (fields[FieldIndex + 1] ~= nil) and (isAttrValid(FileName) == true) then
        local ext = FileName:match(".+%.(.+)$");
        if (ext ~= nil) then
            ext = ext:lower();
            if (commands[ext] ~= nil) then
                local output = getOutput(commands[ext]:gsub("$FILE", FileName));
                return getValue(output, fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]);
            end
        end
        local file = io.open(FileName, "r");
        if (file ~= nil) then
            for line in file:lines() do
                if (string.find(line, fields[FieldIndex + 1][2]) ~= nil) then
                    file:close();
                    return getValue(line, fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]);
                end
            end
            file:close();
        end
    end
    return nil; -- invalid
end

function isAttrValid(filename)
    local attr = SysUtils.FileGetAttr(filename);
    if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
        return false;
    end
    return true;
end

function getOutput(command)
    local handle = io.popen(command, 'r');
    local output = handle:read("*a");
    handle:close();
    return output;
end

function getValue(str, pattern, fieldtype)
    if (str ~= nil) then
        local result = str:match(pattern);
        if (result == nil) and (fieldtype ~= ft_bool) then
            return nil;
        end
        if (fieldtype == ft_float) then
            result = result:gsub(",", "%.");
            result = result:gsub("[^%d%.]", '');
            return tonumber(result);
        elseif (fieldtype == ft_number) then
            result = result:match("%d+");
            return tonumber(result);
        elseif (fieldtype == ft_string) then
            return result;
        elseif (fieldtype == ft_bool) then
            if (result ~= nil) then
                return true;
            else
                return false;
            end
        end
    end
    return nil;
end