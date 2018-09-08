-- dc >= r8316

local ru_en = {
    ["Й"] = "Q", ["й"] = "q", 
    ["Ц"] = "W", ["ц"] = "w", 
    ["У"] = "E", ["у"] = "e", 
    ["К"] = "R", ["к"] = "r", 
    ["Е"] = "T", ["е"] = "t", 
    ["Н"] = "Y", ["н"] = "y", 
    ["Г"] = "U", ["г"] = "u", 
    ["Ш"] = "I", ["ш"] = "i", 
    ["Щ"] = "O", ["щ"] = "o", 
    ["З"] = "P", ["З"] = "p", 
    ["Х"] = "{", ["х"] = "[", 
    ["Ъ"] = "}", ["ъ"] = "]", 

    ["Ф"] = "A", ["ф"] = "a", 
    ["Ы"] = "S", ["ы"] = "s", 
    ["В"] = "D", ["в"] = "d", 
    ["А"] = "F", ["а"] = "f", 
    ["П"] = "G", ["п"] = "g", 
    ["Р"] = "H", ["р"] = "h", 
    ["О"] = "J", ["о"] = "j", 
    ["Л"] = "K", ["л"] = "k", 
    ["Д"] = "L", ["д"] = "l", 
    ["Ж"] = ":", ["ж"] = ";", 
    ["Э"] = '"', ["э"] = "'", 

    ["Я"] = "Z", ["я"] = "z", 
    ["Ч"] = "X", ["ч"] = "x", 
    ["С"] = "C", ["с"] = "c", 
    ["М"] = "V", ["м"] = "v", 
    ["И"] = "B", ["и"] = "b", 
    ["Т"] = "N", ["т"] = "n", 
    ["Ь"] = "M", ["ь"] = "m", 
    ["Б"] = "<", ["б"] = ",", 
    ["Ю"] = ">", ["ю"] = ".", 

    ["Ё"] = "~", ["ё"] = "`", 

    [","] = "?", 
    ['"'] = "@", 
    ["№"] = "#", 
    [";"] = "$", 
    [":"] = "^", 
    ["?"] = "&", 
}

local en_ru = {
    ["Q"] = "Й", ["q"] = "й", 
    ["W"] = "Ц", ["w"] = "ц", 
    ["E"] = "У", ["e"] = "у", 
    ["R"] = "К", ["r"] = "к", 
    ["T"] = "Е", ["t"] = "е", 
    ["Y"] = "Н", ["y"] = "н", 
    ["U"] = "Г", ["u"] = "г", 
    ["I"] = "Ш", ["i"] = "ш", 
    ["O"] = "Щ", ["o"] = "щ", 
    ["P"] = "З", ["p"] = "з", 
    ["{"] = "Х", ["["] = "х", 
    ["}"] = "Ъ", ["]"] = "ъ", 

    ["A"] = "Ф", ["a"] = "ф", 
    ["S"] = "Ы", ["s"] = "ы", 
    ["D"] = "В", ["d"] = "в", 
    ["F"] = "А", ["f"] = "а", 
    ["G"] = "П", ["g"] = "п", 
    ["H"] = "Р", ["h"] = "р", 
    ["J"] = "О", ["j"] = "о", 
    ["K"] = "Л", ["k"] = "л", 
    ["L"] = "Д", ["l"] = "д", 
    [":"] = "Ж", [";"] = "ж", 
    ['"'] = "Э", ["'"] = "э", 

    ["Z"] = "Я", ["z"] = "я", 
    ["X"] = "Ч", ["x"] = "ч", 
    ["C"] = "С", ["c"] = "с", 
    ["V"] = "М", ["v"] = "м", 
    ["B"] = "И", ["b"] = "и", 
    ["N"] = "Т", ["n"] = "т", 
    ["M"] = "Ь", ["m"] = "ь", 
    ["<"] = "Б", [","] = "б", 
    [">"] = "Ю", ["."] = "ю", 

    ["~"] = "Ё", ["`"] = "ё", 

    ["?"] = ",", 
    ["/"] = ".", 
    ["@"] = '"', 
    ["#"] = "№", 
    ["$"] = ";", 
    ["^"] = ":", 
    ["&"] = "?", 
}

function ContentGetSupportedField(Index)
    if (Index == 0) then
        return 'RU > EN', 'Filename.Ext|Filename|Ext', 8; -- FieldName,Units,ft_string
    elseif (Index == 1) then
        return 'EN > RU', 'Filename.Ext|Filename|Ext', 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local result = FileName;
    if (string.find(result, "[" .. SysUtils.PathDelim .. "]%.%.$")) then
        return nil;
    end
    local isDir = SysUtils.DirectoryExists(result);
    if (UnitIndex == 0) then
        result = getFullname(result);
    elseif (UnitIndex == 1) then
        result = getFilename(result, isDir);
    elseif (UnitIndex == 2) then
        result = getExt(result, isDir);
    end
    if (result == nil) then
        return nil;
    elseif (FieldIndex == 0) then
        return strReplace(result, ru_en);
    elseif (FieldIndex == 1) then
        return strReplace(result, en_ru);
    end
    return nil; -- invalid
end

function getFullname(Str)
    if (Str == nil) then
        return nil;
    end
    return string.match(Str, "[" .. SysUtils.PathDelim .. "]([^" .. SysUtils.PathDelim .. "]+)$");
end

function getFilename(Str, IsDir)
    local FileName = nil;
    local FullName = getFullname(Str);
    if (FullName ~= nil) and (IsDir == false) then
        FileName = string.match(FullName, "(.+)%..+");
    end
    if (FileName ~= nil) then
        return FileName;
    else
        return FullName;
    end
end

function getExt(Str, IsDir)
    if (Str == nil) or (IsDir == true) then
        return nil;
    end
    if (getFilename(Str, IsDir) == getFullname(Str)) then
        return nil;
    end
    return string.match(Str, ".+%.(.+)$");
end

function strReplace(Str, Table)
    local Result = '';
    for I = 1, LazUtf8.Length(Str) do
    local Char = LazUtf8.Copy(Str, I, 1);
        if (Table[Char] ~= nil) then
            Result = Result .. Table[Char];
        else
            Result = Result .. Char;
        end
    end
    return Result;
end