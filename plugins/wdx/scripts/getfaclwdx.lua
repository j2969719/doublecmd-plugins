-- requires getfacl

local cmd = "getfacl"
local params = "-E"
local separator = ", "
local nocut = false
local skipsysfiles = true  -- skip character, block or fifo files
local output = ''
local filename = ''

local fields = {
    {"User",           "\nuser::([^\n]+)",          false}, 
    {"Users",          "\nuser:([^\n]+)",            true}, 
    {"Default User",   "\ndefault:user::([^\n]+)",  false}, 
    {"Default Users",  "\ndefault:user:([^\n]+)",    true}, 
    {"Group",          "\ngroup::([^\n]+)",         false}, 
    {"Groups",         "\ngroup:([^\n]+)",           true}, 
    {"Default Group",  "\ndefault:group::([^\n]+)", false}, 
    {"Default Groups", "\ndefault:group:([^\n]+)",   true}, 
    {"Mask",           "\nmask::([^\n]+)",          false}, 
    {"Default Mask",   "\ndefault:mask::([^\n]+)",  false}, 
    {"Other",          "\nother::([^\n]+)",         false}, 
    {"Default Other",  "\ndefault:other::([^\n]+)", false}, 
    {"Flags",          "\n%#%sflags:%s([^\n]+)",    false}, 
    {"Owner Name",     "\n%#%sowner:%s([^\n]+)",    false}, 
    {"Group Name",     "\n%#%sgroup:%s([^\n]+)",    false}, 
}


function ContentGetSupportedField(Index)
    if (fields[Index + 1] ~= nil) then
        return fields[Index + 1][1], "", 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
end

function ContentGetDetectString()
    return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FileName:find("[^" .. SysUtils.PathDelim .. "]%.%.$")) then
        return nil;
    end
    if (filename ~= FileName) then
        if (skipsysfiles == true) then
            local attr = SysUtils.FileGetAttr(FileName);
            if (attr > 0) then
                if (math.floor(attr / 0x00000004) % 2 ~= 0) then
                    return nil; 
                end
            end
        end
        local handle = io.popen(cmd .. ' ' .. params .. ' "' .. FileName .. '"');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    if (fields[FieldIndex + 1][2] ~= nil) and (fields[FieldIndex + 1][3] ~= nil) then
        return getvalue(fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]);
    end 
    return nil;
end

function getvalue(pattern, multi)
    local resultstr = '';
    if (multi == true) then   
        for str in output:gmatch(pattern) do
            if (resultstr ~= '') then
                resultstr = resultstr .. separator .. str;
            elseif (str:sub(1, 1) ~= ":") or (nocut == true) then
                resultstr = str;
            end  
        end        
    else
        resultstr = output:match(pattern);
    end
    return resultstr;
end
