
local commands = {
    ['pdf' ] = 'pdftotext -layout -nopgbrk "$FILE" -', 
    ['djv' ] = 'djvused -e print-pure-txt "$FILE"', 
    ['djvu'] = 'djvused -e print-pure-txt "$FILE"', 
    ['epub'] = 'epub2txt "$FILE"', 
    
    ['doc' ] = 'catdoc -w "$FILE"', 
    ['rtf' ] = 'catdoc -w "$FILE"', 
    ['docx'] = 'docx2txt "$FILE" -', 
    ['odt' ] = 'odt2txt "$FILE"', 
    
    ['xls' ] = 'xls2csv "$FILE"', 
    ['xlsx'] = 'xlsx2csv "$FILE"', 
}

local encoding = {
    "ansi", "oem", 
    "cp1250", "cp1251", "cp1252", "cp1253", "cp1254", "cp1255", "cp1256", "cp1257", "cp1258", 
    "cp437", "cp850", "cp852", "cp866", "cp874", "cp932", "cp936", "cp949", "cp950", 
    "iso88591", "iso88592", "iso885915", 
    "macintosh", "koi8", 
    "ucs2le", "ucs2be", 
}

local convert = nil;

if (LazUtf8 ~= nil) then
    convert = LazUtf8.ConvertEncoding;
end

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'Text', '', 9; -- FieldName,Units,ft_fulltext
    elseif (convert ~= nil) and (encoding[FieldIndex] ~= nil) then
        return 'Text (' .. encoding[FieldIndex] .. ')', '', 9; -- FieldName,Units,ft_fulltext
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    local detect_string = '';
    for ext in pairs(commands) do
        if (detect_string == '') then
            detect_string = '(EXT="' .. ext:upper() .. '")';
        else
            detect_string = detect_string .. ' | (EXT="' .. ext:upper() .. '")';
        end
    end
    return detect_string; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if not SysUtils.DirectoryExists(FileName) and (UnitIndex == 0) then
        local ext = FileName:match(".+%.(.+)$");
        if (ext ~= nil) then
            ext = ext:lower();
            if (commands[ext] ~= nil) then
                local handle = io.popen(commands[ext]:gsub("$FILE", FileName), 'r');
                local result = handle:read("*a");
                handle:close();
                if (FieldIndex > 0) and (encoding[FieldIndex] ~= nil) then
                    return convert(result, encoding[FieldIndex], 'utf8');
                else
                    return result;
                end
            end
        end
    end
    return nil; -- invalid
end
