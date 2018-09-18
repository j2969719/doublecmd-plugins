-- hexdecheaderwdx.lua

local skipsysfiles = true  -- skip system files (character, block or fifo files on linux)
local showoffset = true    -- show offset near field name

local values = {
    "66 74 79 70 69 73 6F 6D", 
    "ftypisom", 
}

local fields = {
  -- field name,    offset,  number of bytes,  type,   result(boolean field)
    {"header",      "0x0",       8,           "hex",         nil}, 
    {"header",      "0x0",       15,          "dec",         nil}, 
    {"header",      "0x0",       4,           "raw",         nil}, 
    {"some offset", "0x20B0",    16,          "hex",         nil}, 
    {"test check",  "0x4",       8,           "hex",   values[1]}, 
    {"test check",  "0x4",       8,           "raw",   values[2]}, 
}

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil) then
        local fieldname = fields[FieldIndex + 1][1];
        if (tonumber(fields[FieldIndex + 1][2]) > 0) and (showoffset == true) then
            fieldname = fieldname .. " " .. fields[FieldIndex + 1][2];
            if (fields[FieldIndex + 1][5] ~= nil) and (fields[FieldIndex + 1][4] ~= "raw") then
                fieldname = fieldname .. ": " .. fields[FieldIndex + 1][5];
            end
        end
        if (fields[FieldIndex + 1][4] ~= "hex") then
            fieldname = fieldname .. " (" .. fields[FieldIndex + 1][4] .. ")";
        end
        if (fields[FieldIndex + 1][5] ~= nil) then
            return fieldname, '', 6;
        else
            return fieldname, getUnitsString(fields[FieldIndex + 1][3]), 8;
        end
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local attr = SysUtils.FileGetAttr(FileName);
    if (isWrongAttr(attr) == true) then
        return nil;
    end
    local f = io.open(FileName, "rb");
    local result = nil;
    if (f ~= nil) then
        f:seek("set", tonumber(fields[FieldIndex + 1][2]));
        if (fields[FieldIndex + 1][5] ~= nil) then
            bytes = f:read(fields[FieldIndex + 1][3]);
            if (getBytes(fields[FieldIndex + 1][4], bytes) == fields[FieldIndex + 1][5]) then
                result = true;
            else
                result = false;
            end
        else
            bytes = f:read(UnitIndex + 1);
            result = getBytes(fields[FieldIndex + 1][4], bytes);    
        end
        f:close();
    end
    return result;
end

function getFormattedString(formatstring, bytes)
    if (bytes == nil) then
        return nil;
    else
        local t = {};
        for b in string.gfind(bytes, ".") do
            table.insert(t, string.format(formatstring, string.byte(b)));
        end
        local result = table.concat(t);
        return result;
    end
end

function getBytes(ftype, bytes)
    if (ftype == "raw") then
        return bytes;
    end
    local formatstring = "%02X ";
    if (ftype == "dec") then
        formatstring = "%03d ";
    end
    local result = getFormattedString(formatstring, bytes);
    if (result ~= nil) then
        return result:sub(1, - 2);
    end
    return nil;
end

function getUnitsString(num)
    if (num < 1) then
        return '';
    end
    local result = "1 byte";
    if (num > 1) then
        for i = 2, num do
            result = result .. '|' .. i .. " bytes";
        end
    end
    return result;
end

function isWrongAttr(vattr)
    if (vattr < 0) or (math.floor(vattr / 0x00000010) % 2 ~= 0) then
        return true;
    elseif (math.floor(vattr / 0x00000004) % 2 ~= 0) and (skipsysfiles == true) then
        return true;
    end
    return false;
end
