local cmd = "getfattr"
local params = "--absolute-names -d"
local output = ''
local dosattr = ''
local filename = ''

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'archive', '', 6; 
    elseif (Index == 1) then
        return 'compressed', '', 6;
    elseif (Index == 2) then
        return 'encrypted', '', 6;
    elseif (Index == 3) then
        return 'hidden', '', 6;
    elseif (Index == 4) then
        return 'readonly', '', 6;
    elseif (Index == 5) then
        return 'system', '', 6;
    elseif (Index == 6) then
        return 'attr string', '', 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return nil; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local delimpat = SysUtils.PathDelim;
    if (delimpat == nil) then
        delimpat = "/\\";
    end
    if (FileName:find("[^" .. delimpat .. "]%.%.$")) then
        return nil;
    end
    if (filename ~= FileName) or (FieldIndex == 6) then
        local attr = SysUtils.FileGetAttr(FileName);      
        if (attr < 0) or (checkattr(attr, 0x00000004)) then
            return nil;
        end
        local handle = io.popen(cmd .. ' ' .. params .. ' "' .. FileName .. '"', 'r');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
        if (output ~= nil) then
            dosattr = output:match('%.DOSATTRIB="([^"]+)');
        end
    end
    if (FieldIndex == 0) then
        return checkattr(dosattr, 0x20);
    elseif (FieldIndex == 1) then
        return checkattr(dosattr, 0x800);
    elseif (FieldIndex == 2) then
        return checkattr(dosattr, 0x4000);
    elseif (FieldIndex == 3) then
        return checkattr(dosattr, 0x2);
    elseif (FieldIndex == 4) then
        return checkattr(dosattr, 0x1);
    elseif (FieldIndex == 5) then
        return checkattr(dosattr, 0x4);
    elseif (FieldIndex == 6) then
        local str = '';       
        if (checkattr(dosattr, 0x1)) then
            str = str .. 'r';
        else
            str = str .. '-';
        end
        if (checkattr(dosattr, 0x20)) then
            str = str .. 'a';
        else
            str = str .. '-';
        end
        if (checkattr(dosattr, 0x2)) then
            str = str .. 'h';
        else
            str = str .. '-';
        end
        if (checkattr(dosattr, 0x4)) then
            str = str .. 's';
        else
            str = str .. '-';
        end
        return str;
    end
    return nil; -- invalid
end

function checkattr(vattr, val)
    if (vattr ~= nil) then
        if (math.floor(vattr / val) % 2 ~= 0) then
            return true;        
        end
    end
    return false; 
end
