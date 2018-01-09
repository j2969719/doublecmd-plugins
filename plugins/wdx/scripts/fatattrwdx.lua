local cmd = "fatattr"
local output = ''
local dosattr = ''
local filename = ''

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'archive', '', 6; 
    elseif (Index == 1) then
        return 'hidden', '', 6;
    elseif (Index == 2) then
        return 'readonly', '', 6;
    elseif (Index == 3) then
        return 'system', '', 6;
    elseif (Index == 4) then
        return 'attr string', '', 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return nil; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (string.find(FileName, "[^" .. SysUtils.PathDelim .. "]%.%.$")) then
        return nil;
    end
    if (filename ~= FileName) or (FieldIndex == 4) then        
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr > 0) then
            if (math.floor(attr / 0x00000004) % 2 ~= 0)  then
                return nil; 
            end
        end        
        local handle = io.popen(cmd .. ' "' .. FileName .. '"');
        output = handle:read("*a");
        handle:close();
        if (output:sub(1, 9) ~= FileName:sub(1, 9)) then
            dosattr = output:sub(1, 9);
        end
        filename = FileName;
    end
    if (FieldIndex == 0) then
        return chechattr(dosattr, 'a');
    elseif (FieldIndex == 1) then
        return chechattr(dosattr, 'h');
    elseif (FieldIndex == 2) then
        return chechattr(dosattr, 'r');
    elseif (FieldIndex == 3) then
        return chechattr(dosattr, 's');
    elseif (FieldIndex == 4) then
        local str = '';       
        if (chechattr(dosattr, 'r')) then
            str = str .. 'r';
        else
            str = str .. '-';
        end
        if (chechattr(dosattr, 'a')) then
            str = str .. 'a';
        else
            str = str .. '-';
        end
        if (chechattr(dosattr, 'h')) then
            str = str .. 'h';
        else
            str = str .. '-';
        end
        if (chechattr(dosattr, 's')) then
            str = str .. 's';
        else
            str = str .. '-';
        end
        return str;
    end
    return nil; -- invalid
end

function chechattr(vattr, val)
    if (vattr ~= nil) then
        if (string.find(vattr, val)) then
            return true;
        end
    end
    return false;   
end
