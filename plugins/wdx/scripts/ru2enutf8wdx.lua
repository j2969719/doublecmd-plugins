-- ru2enutf8wdx.lua
-- luarocks-5.1 install luautf8

local utf8 = require("lua-utf8")

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
--    ["."] = "/", -- useless
    ['"'] = "@", 
    ["№"] = "#", 
    [";"] = "$", 
    [":"] = "^", 
    ["?"] = "&", 
--    ["/"] = "|" -- useless
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
--    ["|"] = "/" -- useless
} 

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'RU > EN', 'Filename.Ext|Filename|Ext', 8; -- FieldName,Units,ft_string
    elseif (FieldIndex == 1) then
        return 'EN > RU', 'Filename.Ext|Filename|Ext', 8;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local result = FileName;
    if (utf8.find(result, "[/\\]%.%.$")) then
        return nil;
    end
    if (UnitIndex == 0) then
        result = utf8.match(result, "^.*[/\\](.+[^/\\])$");
    elseif (UnitIndex == 1) then
        result = utf8.match(result, "^.*[/\\](.+[^/\\])$");
        if (result ~= nil) and (utf8.match(result, "(.+)%..+")) then
            result = utf8.match(result, "(.+)%..+");
        end
    elseif (UnitIndex == 2) then
        result = utf8.match(result, ".+%.(.+)$");
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

function strReplace(str, tbl)
    local result = '';
    for char in utf8.gmatch(str, ".") do 
        if (tbl[char] ~= nil) then
            result = result .. tbl[char];
        else
            result = result .. char;
        end 
    end
    return result;
end