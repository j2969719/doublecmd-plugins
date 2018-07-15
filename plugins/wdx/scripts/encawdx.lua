-- requires enca

local cmd = "enca"
local params = ''
local cacheoutput = false
local output = ''
local filename = ''

local values = {    -- return values
    {"UTF-8", "CP1251", "IBM866", "KOI8-R", "ISO-8859-5", "UTF-7", "CP1252", "ASCII", "???"},
}

local fields = {    -- field name, command line parameters, pattern, return values
    {"iconv name",               "-g -i",    "([^\n]+)",       nil},
    {"iconv name(multichoice)",  "-g -i",    "([^\n]+)", values[1]},
    {"enca's encoding name",     "-g -e",    "([^\n]+)",       nil},
    {"lang ru",                  "-L ru -g",  "(.+)\n$",       nil},
}

function ContentGetSupportedField(Index)
    if (fields[Index + 1] ~= nil) then
        if (fields[Index + 1][4] ~= nil) then
            local units = '';
            for i = 1 , #fields[Index + 1][4] do
                if (i > 1) then
                    units = units .. '|';
                end
                units = units .. fields[Index + 1][4][i];
            end
            return fields[Index + 1][1], units, 7;
        else
            return fields[Index + 1][1], "", 8;
        end
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return nil; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (filename ~= FileName) or (params ~= fields[FieldIndex + 1][2]) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            return nil;
        end
        params = fields[FieldIndex + 1][2];
        local handle = io.popen(cmd .. ' ' .. params .. ' "' .. FileName .. '"');
        output = handle:read("*a");
        handle:close();
        if (cacheoutput == true) then
            filename = FileName;
        end
    end
    if (fields[FieldIndex + 1][3] ~= nil) then
        return output:match(fields[FieldIndex + 1][3]);
    end
    return nil;
end
