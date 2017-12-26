-- duwdx.lua

local decimalPlaces = 2
local apparentSize = true
local skipOtherFS = false
local isSI = false

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'Bytes', '', 2; -- FieldName,Units,ft_string
    elseif (Index == 1) then
        return 'K', '', 3;
    elseif (Index == 2) then
        return 'M', '', 3;
    elseif (Index == 3) then
        return 'G', '', 3;
    elseif (Index == 4) then
        return 'T', '', 3;
    elseif (Index == 5) then
        return 'Human readable', '', 8;
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
    local cmd1 = "du -s";
    local cmd2 = "du -sh";
    local bval = 1024;
    if (skipOtherFS == true) then
        cmd1 = cmd1 .. 'x';
        cmd2 = cmd2 .. 'x';
    end
    if (apparentSize == true) then
        cmd1 = cmd1 .. 'b';
        cmd2 = cmd2 .. ' --apparent-size';
    else
        cmd1 = cmd1 .. 'k';
    end
    if (isSI == true) then
        bval = 1000;
        cmd2 = cmd2 .. ' --si';
    end
    if (string.sub(FileName, - 3) == "/..") or (string.sub(FileName, - 2) == "/.") then
        return nil;
    end
    if (FieldIndex < 5) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr > 0) then
            if (math.floor(attr / 0x00000004) % 2 ~= 0)  then
                return nil; 
            end
        end        
        local handle = io.popen(cmd1 .. ' "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        result = result:match("^%d+");
        if (FieldIndex == 0) then
            return result;
        else
            return szcalc(result, bval, FieldIndex); 
        end
    elseif (FieldIndex == 5) then
        local handle = io.popen(cmd2 .. ' "'..FileName..'"');
        local result = handle:read("*a");
        handle:close();
        return result:match("^%d+.?%d+%w");
    end
    return nil; -- invalid
end

function round(num, numDecimalPlaces)
    local mult = 10 ^ (numDecimalPlaces or 0);
    return math.floor(num * mult + 0.5) / mult;
end

function szcalc(num, bytes, pov)
    local res = num / bytes ^ pov;
    return round(res, decimalPlaces);
end
