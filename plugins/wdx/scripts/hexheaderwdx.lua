-- hexheaderwdx.lua

local skipsysfiles = true -- skip system files (character, block or fifo files on linux)

local offset = {
    "0x10",
    "0x000020",
    "0x20B0",
}

function ContentGetSupportedField(Index)
    local submi = "1 byte";
    for i = 2, 16 do
        submi = submi .. '|' .. i .. " bytes";
    end
    if (Index == 0) then
        return "header", submi, 8; -- FieldName,Units,ft_string
    elseif (Index == 1) then
        return "all file", '', 9; -- FieldName,Units,ft_fulltext
    elseif (offset[Index - 1] ~= nil) then
        return "offset: " .. offset[Index - 1], submi, 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return nil; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local attr = SysUtils.FileGetAttr(FileName);
    if (isWrongAttr(attr) == true) then
        return nil;
    end
    local f = io.open(FileName, "rb");
    local step = 1024;
    local result = nil;
    if (f ~= nil) then
        if (FieldIndex == 0) then
            result = toHex(f:read(UnitIndex + 1));
        elseif (FieldIndex == 1) then
           if (UnitIndex == 0) then
             result = toHexFull(f:read(step));
           else
             -- Переводим UnitIndex в байты
             -- на каждый байт 3 символа в Hex строке
             f:seek("set", UnitIndex / 3);
             result = toHexFull(f:read(step));
           end
        else
           f:seek("set", tonumber(offset[FieldIndex - 1]));
           result = toHex(f:read(UnitIndex + 1));
        end
        f:close();
    end
    return result;
end

-- Результат с завершающим пробелом
-- В итоге 1 байт = 3 символа в строке
function toHexFull(bytes)
    if (bytes == nil) then
        return nil;
    else
      local t = {}
      for b in string.gfind(bytes, ".") do
        table.insert(t, string.format("%02X ", string.byte(b)));
      end
      local result = table.concat(t);
      return result;
    end
end

function toHex(bytes)
    local result = toHexFull(bytes);
    if (result ~= nil) then
        return result:sub(1, - 2);
    end
    return nil;
end

function isWrongAttr(vattr)
    if (vattr < 0) or (math.floor(vattr / 0x00000010) % 2 ~= 0) then
        return true;
    elseif (math.floor(vattr / 0x00000004) % 2 ~= 0) and (skipsysfiles == true) then
        return true;
    end
    return false;
end
