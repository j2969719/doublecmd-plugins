
local commands = {
    ['pdf' ] = 'pdftotext -layout -nopgbrk "$FILE" -',
    ['djv' ] = 'djvused -e print-pure-txt -u "$FILE"',
    ['djvu'] = 'djvused -e print-pure-txt -u "$FILE"',
    ['epub'] = 'epub2txt "$FILE"',
    ['fb2' ] = 'ebook2text "$FILE"',

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
    "cp437", "cp850", "cp852", "cp865", "cp866", "cp874", "cp932", "cp936", "cp949", "cp950",
    "iso88591", "iso88592", "iso88593", "iso88594", "iso88595", "iso88597", "iso88599", "iso885910", "iso885913", "iso885914", "iso885915", "iso885916",
    "macintosh", "koi8r",
    "ucs2le", "ucs2be",
}

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'Text', '', 9 -- FieldName,Units,ft_fulltext
    elseif (LazUtf8 ~= nil) and (encoding[FieldIndex] ~= nil) then
        return 'Text (' .. encoding[FieldIndex] .. ')', '', 9 -- FieldName,Units,ft_fulltext
    end
    return '', '', 0 -- ft_nomorefields
end

function ContentGetDetectString()
    local detect_string = ''
    for ext in pairs(commands) do
        if (detect_string == '') then
            detect_string = '(EXT="' .. ext:upper() .. '")'
        else
            detect_string = detect_string .. ' | (EXT="' .. ext:upper() .. '")'
        end
    end
    return detect_string -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if not SysUtils.DirectoryExists(FileName) and (UnitIndex == 0) then
        local ext = FileName:match(".+%.(.+)$")
        if (ext ~= nil) then
            ext = ext:lower()
            if (commands[ext] ~= nil) then
                local handle = io.popen(commands[ext]:gsub("$FILE", function() return FileName:gsub('"', '\\"') end), 'r')
                local result = handle:read("*a")
                handle:close()
                if (FieldIndex > 0) and (encoding[FieldIndex] ~= nil) then
                    return LazUtf8.ConvertEncoding(result, encoding[FieldIndex], 'utf8')
                else
                    return result
                end
            end
        end
    end
    return nil -- invalid
end