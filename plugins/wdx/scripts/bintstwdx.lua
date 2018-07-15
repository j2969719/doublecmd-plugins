local skipsysfiles = true  -- skip character, block or fifo files
local blacklist = '\a\b\t\n\f\r\\e'
local bufsize = 1024
local units = {
    "binary (contain control chars)", 
    "UTF-32, big-endian (byte order mark)", 
    "UTF-32, little-endian (byte order mark)", 
    "UTF-16, big-endian (byte order mark)", 
    "UTF-16, little-endian (byte order mark)", 
    "UTF-8 (byte order mark)", 
}

function ContentGetSupportedField(Index)
    if (Index == 0) then
        local unitstr = '';
        for i = 1 , #units do
            if (i > 1) then
                unitstr = unitstr .. '|';
            end
            unitstr = unitstr .. units[i];
        end
        return 'content', unitstr, 7;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FieldIndex == 0) then
        if (skipsysfiles == true) then
            local attr = SysUtils.FileGetAttr(FileName);
            if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) then
                return nil; 
            end
        end    
        local isbin = false;
        local f = io.open(FileName, "rb");
        if (f ~= nil) then
            if checkbom(f, 4, '0000FEFF') then
                return units[2];
            elseif checkbom(f, 4, 'FFFE0000') then
                return units[3];
            elseif checkbom(f, 2, 'FEFF') then
                return units[4];
            elseif checkbom(f, 2, 'FFFE') then
                return units[5];
            elseif checkbom(f, 3, 'EFBBBF') then
                return units[6];
            end
            f:seek("set", 0);
            while (isbin ~= true) do
                local buf = f:read(bufsize);
                if (buf == nil) then 
                    break;
                end
                for b in buf:gfind("%c") do
                    if (blacklist:find(b) == nil) then
                        --local val = b:byte(); 
                        --print(string.format("found control char %02X (%d)", val, val));
                        isbin = true;
                        break;
                    end
                end
            end
            f:close();
            if (isbin == true) then
                return units[1];
            end
        end  
    end
    return nil; -- invalid
end

function checkbom(file, num, hexstr)
    file:seek("set", 0);
    local bytes = file:read(num);
    if (bytes ~= nil) then
        local t = {};
        for b in string.gfind(bytes, ".") do
            table.insert(t, string.format("%02X", string.byte(b)));
        end       
        local str = table.concat(t);
        if (hexstr == str) then
            return true;
        end
    end
    return false;
end