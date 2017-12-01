-- ru2enwdx.lua

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
    ["Х"] = "{", ["х"] = "%[", 
    ["Ъ"] = "}", ["ъ"] = "%]", 
    
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
    ["Э"] = "\"", ["э"] = "'",

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
    
    [";"] = "%$",
    [":"] = "%^",
    ["%?"] = "&",
    ["\""] = "@",
    ["№"] = "#"
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
    ["{"] = "Х", ["%["] = "х", 
    ["}"] = "Ъ", ["%]"] = "ъ", 
    
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
    ["\""] = "Э", ["'"] = "э",

    ["Z"] = "Я", ["z"] = "я",
    ["X"] = "Ч", ["x"] = "ч",
    ["C"] = "С", ["c"] = "с",
    ["V"] = "М", ["v"] = "м",
    ["B"] = "И", ["b"] = "и",
    ["N"] = "Т", ["n"] = "т",
    ["M"] = "Ь", ["m"] = "ь",
    ["<"] = "Б", [","] = "б",
    [">"] = "Ю", ["%."] = "ю",

    ["~"] = "Ё", ["`"] = "ё"
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
    if (string.find(result, "[/\\]%.%.$")) then
        return nil;
    end
    if (UnitIndex == 0) then
        result = string.match(result, "^.*[/\\](.+[^/\\])$");
    elseif (UnitIndex == 1) then
        result = string.match(result, "^.*[/\\](.+[^/\\])$");
        if (result ~= nil) and (string.match(result, "(.+)%..+")) then
            result = string.match(result, "(.+)%..+");
        end
    elseif (UnitIndex == 2) then
        result = string.match(result, ".+%.(.+)$");
    end 
    if (result == nil) then
        return nil;
    elseif (FieldIndex == 0) then
        for ru, en in pairs(ru_en) do
            result = string.gsub(result, ru, en);
        end
        return result;
    elseif (FieldIndex == 1) then
        for en, ru in pairs(en_ru) do
            result = string.gsub(result, en, ru);
        end
        return result;
    end
    return nil; -- invalid
end

