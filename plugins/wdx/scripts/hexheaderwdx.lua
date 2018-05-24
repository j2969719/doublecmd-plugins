-- hexheaderwdx.lua

local offset = {
    "0x10",
    "0x000020",
    "0x20B0",
}

function ContentGetSupportedField(Index)
    local submi = '1 byte';
    for i = 2, 16 do
        submi = submi .. '|' .. i .. ' bytes'
    end
    if (Index == 0) then
        return 'header', submi, 8; -- FieldName,Units,ft_string
    elseif (Index == 1) then
        return 'all file', '', 9;
    elseif (offset[Index - 1] ~= nil) then
        return 'offset: ' .. offset[Index - 1], submi, 8;
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
    if (SysUtils.DirectoryExists(FileName)) then
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
        return "";
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
    return result:sub(1, - 2);
end
