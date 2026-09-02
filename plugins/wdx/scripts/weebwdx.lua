local shiet = {
    ["　"] = " ", ["！"] = "!", ["”"] = '"',  ["＃"] = "#", ["＄"] = "$", 
    ["％"] = "%", ["＆"] = "&", ["’"] = "'",  ["（"] = "(", ["）"] = ")", 
    ["＊"] = "*", ["＋"] = "+", ["，"] = ",", ["－"] = "-", ["．"] = ".", 
    ["／"] = "/", ["："] = ":", ["；"] = ";", ["＜"] = "<", ["＝"] = "=", 
    ["＞"] = ">", ["？"] = "?", ["＠"] = "@", ["［"] = "[", ["＼"] = "\\", 
    ["］"] = "]", ["＾"] = "^", ["＿"] = "_", ["｀"] = "`", ["｛"] = "{", 
    ["｜"] = "|", ["｝"] = "}", ["〜"] = "~",

    ["０"] = "0", ["１"] = "1", ["２"] = "2", ["３"] = "3", ["４"] = "4", 
    ["５"] = "5", ["６"] = "6", ["７"] = "7", ["８"] = "8", ["９"] = "9",

    ["Ａ"] = "A", ["Ｂ"] = "B", ["Ｃ"] = "C", ["Ｄ"] = "D", ["Ｅ"] = "E", 
    ["Ｆ"] = "F", ["Ｇ"] = "G", ["Ｈ"] = "H", ["Ｉ"] = "I", ["Ｊ"] = "J", 
    ["Ｋ"] = "K", ["Ｌ"] = "L", ["Ｍ"] = "M", ["Ｎ"] = "N", ["Ｏ"] = "O", 
    ["Ｐ"] = "P", ["Ｑ"] = "Q", ["Ｒ"] = "R", ["Ｓ"] = "S", ["Ｔ"] = "T", 
    ["Ｕ"] = "U", ["Ｖ"] = "V", ["Ｗ"] = "W", ["Ｘ"] = "X", ["Ｙ"] = "Y", 
    ["Ｚ"] = "Z",

    ["ａ"] = "a", ["ｂ"] = "b", ["ｃ"] = "c", ["ｄ"] = "d", ["ｅ"] = "e", 
    ["ｆ"] = "f", ["ｇ"] = "g", ["ｈ"] = "h", ["ｉ"] = "i", ["ｊ"] = "j", 
    ["ｋ"] = "k", ["ｌ"] = "l", ["ｍ"] = "m", ["ｎ"] = "n", ["ｏ"] = "o", 
    ["ｐ"] = "p", ["ｑ"] = "q", ["ｒ"] = "r", ["ｓ"] = "s", ["ｔ"] = "t", 
    ["ｕ"] = "u", ["ｖ"] = "v", ["ｗ"] = "w", ["ｘ"] = "x", ["ｙ"] = "y", 
    ["ｚ"] = "z"
}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "shiet", '', 8 -- FieldName,Units,ft_string
  elseif FieldIndex == 1 then
    return "is shiet", '', 6 -- FieldName,Units,ft_boolean
  end
  return '', '', 0 -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  local result = ""
  local text = SysUtils.ExtractFileName(FileName):sub(1, -(SysUtils.ExtractFileExt(FileName):len() + 1))
  for _, char in LazUtf8.Next(text) do
    if FieldIndex == 0 then
      if not shiet[char] then
        result = result .. char
      else
        result = result .. shiet[char]
      end
    else
      result = false
      if shiet[char] then
        result = true
        break
      end
    end
  end
  return result
end
