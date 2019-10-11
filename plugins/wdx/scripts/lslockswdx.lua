local filename = '';

local fields = {
    {"COMMAND", 8}, 
    {"PID",     2}, 
    {"TYPE",    8}, 
    {"BLOCKER", 8}, 
    {"MODE",    8}, 
    {"M",       2}, 
    {"START",   2}, 
    {"END",     2}, 
}

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        return fields[FieldIndex + 1][1], '', fields[FieldIndex + 1][2];
    end
    
    return '', '', 0; -- ft_nomorefields
end

local function escape_str(str)
    local magic_chars = {"%", ".", "-", "+", "*", "?", "^", "$", "(", ")", "["};
    
    for i, chr in pairs(magic_chars) do
        str = str:gsub("%" .. chr, "%%%" .. chr);
    end   
    
    return str; 
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (filename ~= FileName) then
        local handle = io.popen('lslocks -u -o COMMAND,PID,TYPE,BLOCKER,MODE,M,START,END,PATH', 'r');
        local output = handle:read("*a");
        handle:close();
        filename = FileName;
        
        if (output ~= nil) then
            local pattern = '\n(%w+)%s+(%d+)%s+(%u+)%s+(%w*)%s+(%u+)%s+(%d+)%s+(%d+)%s+(%d+) ' .. escape_str(FileName) .. '\n';
            fields[1][3], 
            fields[2][3], 
            fields[3][3], 
            fields[4][3], 
            fields[5][3], 
            fields[6][3], 
            fields[7][3], 
            fields[8][3] = output:match(pattern);
        else
            for i = 1, #fields do
                fields[i][3] = nil;
            end
        end
        
    end
    
    if (fields[FieldIndex + 1] ~= nil) then
        return fields[FieldIndex + 1][3];
    end
    
    return nil; -- invalid
end