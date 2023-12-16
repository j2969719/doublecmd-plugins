-- changecasewdx.lua (cross-platform)
--2023.12.16
--[[
Changing the case of letters

Values:
- lowercase
  all letters are lowercase;
- UPPERCASE
  all letters are uppercase;
- First Uppercase
  the first letter of the string is uppercase;
- Sentence case
  the first letter of the string is uppercase, the others are lowercase;
- Start Case
  the first letter of each word is uppercase, the others are lowercase.

Script takes into account:
- all letters (L* Unicode categories);
- accent, umlaut and other combining diacritical marks (U+0300...U+036F from the Mn Unicode category);
- apostrophe (U+0027 and U+2019);
- hyphen (U+2010, but will skip U+002D).

"Base name": file name without extension (regular files) or entire name (folders).
]]

local aU8 = {}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Base name", "lowercase|UPPERCASE|First Uppercase|Sentence case|Start Case", 8
  elseif FieldIndex == 1 then
    return "Extension", "lowercase|UPPERCASE", 8
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 1 then return nil end
  local iA, sN, sB, sE
  sN = SysUtils.ExtractFileName(FileName)
  iA = SysUtils.FileGetAttr(FileName)
  if math.floor(iA / 0x00000010) % 2 ~= 0 then
    sB = sN
    sE = ""
  else
    sB = string.gsub(sN, "^(.+)%.[^%.]+$", "%1")
    sE = string.sub(sN, string.len(sB) + 2, -1)
  end
  if FieldIndex == 0 then
    -- lowercase
    if UnitIndex == 0 then
      return LazUtf8.LowerCase(sB)
    -- UPPERCASE
    elseif UnitIndex == 1 then
      return LazUtf8.UpperCase(sB)
    else
      local iP = 0
      local sT1, sT2, sT3
      GetTableUTF8Char(sB)
      -- First or Sentence case
      if (UnitIndex == 2) or (UnitIndex == 3) then
        for i = 1, #aU8 do
          iA = Char.GetUnicodeCategory(aU8[i])
          if iA < 5 then
            iP = i
            break
          end
        end
        if iP == 0 then return sB end
        if iP == 1 then
          sT1 = ""
          sT2 = LazUtf8.UpperCase(aU8[1])
          sT3 = table.concat(aU8, "", 2, #aU8)
        elseif iP > 1 then
          sT1 = table.concat(aU8, "", 1, iP - 1)
          sT2 = LazUtf8.UpperCase(aU8[iP])
          sT3 = table.concat(aU8, "", iP + 1, #aU8)
        end
        if UnitIndex == 2 then
          return sT1 .. sT2 .. sT3
        elseif UnitIndex == 3 then
          return sT1 .. sT2 .. LazUtf8.LowerCase(sT3)
        end
      -- Start Case
      elseif UnitIndex == 4 then
        return StartCase(sB)
      end
    end
  elseif FieldIndex == 1 then
    if sE == "" then return nil end
    if UnitIndex == 0 then
      return LazUtf8.LowerCase(sE)
    elseif UnitIndex == 1 then
      return LazUtf8.UpperCase(sE)
    end
  end
  return nil
end

function GetTableUTF8Char(s)
  local l = string.len(s)
  local i, j = 1, 1
  local n
  -- rfc3629
  while true do
    if i > l then break end
    n = string.byte(s, i)
    if (n >= 0) and (n <= 127) then
      aU8[j] = string.sub(s, i, i)
      i = i + 1
    elseif (n >= 194) and (n <= 223) then
      aU8[j] = string.sub(s, i, i + 1)
      i = i + 2
    elseif (n >= 224) and (n <= 239) then
      aU8[j] = string.sub(s, i, i + 2)
      i = i + 3
    elseif (n >= 240) and (n <= 244) then
      aU8[j] = string.sub(s, i, i + 3)
      i = i + 4
    end
    j = j + 1
  end
  if #aU8 >= j then
    for i = #aU8, j, -1 do aU8[i] = nil end
  end
end

function StartCase(s)
  local iT, iP
  local aT1 = {}
  local aT2 = {}
  local sU = LazUtf8.ConvertEncoding(s, "utf8", "ucs2be")
  for i = 1, #aU8 do
    iT = Char.GetUnicodeCategory(aU8[i])
    -- Letters (L*)
    if iT < 5 then
      aT1[i] = 1
    else
      iP = (i - 1) * 2
      if iT == 5 then
        -- Accent, umlaut and other combining diacritical marks, U+0300...U+036F (Mn)
        if (string.byte(sU, iP + 1) == 0x03) and (string.byte(sU, iP + 2) < 0x70) then
          aT1[i] = 1
        end
      else
        -- Apostrophe: U+0027, U+2019
        if string.char(0x27) == aU8[i] then
          if aT1[i - 1] == 1 then aT1[i] = 1 end
        end
        if (string.byte(sU, iP + 1) == 0x20) and (string.byte(sU, iP + 2) == 0x19) then
          if aT1[i - 1] == 1 then aT1[i] = 1 end
        end
        -- Hyphen: U+002D, U+2010
--[[
        if string.char(0x2d) == aU8[i] then
          aT1[i] = 1
        end
]]
        if (string.byte(sU, iP + 1) == 0x20) and (string.byte(sU, iP + 2) == 0x10) then
          aT1[i] = 1
        end
        if aT1[i] ~= 1 then aT1[i] = 0 end
      end
    end
  end
  if aT1[1] == 1 then
    aT2[1] =  LazUtf8.UpperCase(aU8[1])
  else
    aT2[1] = aU8[1]
  end
  for i = 2, #aT1 do
    if aT1[i] == 1 then
      if aT1[i - 1] == 1 then
        aT2[i] = LazUtf8.LowerCase(aU8[i])
      else
        aT2[i] = LazUtf8.UpperCase(aU8[i])
      end
    else
      aT2[i] = aU8[i]
    end
  end
  return table.concat(aT2, "")
end
