-- jpeginfowdx.lua (cross-platform)
-- 2020.10.30
--[[
Returns some info from JPEG.
See list of supported fields in table "fields".
"JPEG compression" field will return text, other fields are boolean (true if marker exists).

ITU T.81/IEC 10918-1 JPEG (1994)
ITU T.84/IEC 10918-3 JPEG extensions (1997)
ITU T.87/IEC 14495-1 JPEG LS
]]

local fields = {
{"JFIF",              "", 6},
{"JFXX",              "", 6},
{"EXIF",              "", 6},
{"XMP",               "", 6},
{"ICC color profile", "", 6},
{"Adobe Photoshop IRB, IPTC", "", 6},
{"Adobe segment",     "", 6},
{"Comment",           "", 6},
{"JPEG compression",  "", 8}
}

local compres = {
{"SOF0",  0xc0, "Baseline DCT"},
{"SOF1",  0xc1, "Extended sequential DCT"},
{"SOF2",  0xc2, "Progressive DCT"},
{"SOF3",  0xc3, "Lossless (sequential)"},
{"SOF5",  0xc5, "Differential sequential DCT"},
{"SOF6",  0xc6, "Differential progressive DCT"},
{"SOF7",  0xc7, "Differential lossless (sequential)"},
{"SOF9",  0xc9, "Extended sequential DCT, arithmetic coding"},
{"SOF10", 0xca, "Progressive DCT, arithmetic coding"},
{"SOF11", 0xcb, "Lossless (sequential), arithmetic coding"},
{"SOF13", 0xcd, "Differential sequential DCT, arithmetic coding"},
{"SOF14", 0xce, "Differential progressive DCT, arithmetic coding"},
{"SOF15", 0xcf, "Differential lossless (sequential), arithmetic coding"},
{"SOF48", 0xf7, "JPEG-LS baseline"},
{"LSE",   0xf8, "JPEG-LS baseline"},
{"SOF57", 0xf9, "JPEG LS extensions"}
}

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]
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
  if FieldIndex >= #fields then return nil end
  local at = SysUtils.FileGetAttr(FileName)
  if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
  local ar = {
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    nil
  }
  local d, n, t
  local p = 0
  local h = io.open(FileName, 'rb')
  if h == nil then return nil end
  local d = h:read(2)
  -- magic: ff d8
  if (d == nil) or (BinToNum(d, 1, 2) ~= 65496) then
    h:close()
    return nil
  end
  while true do
    d = h:read(4)
    if string.byte(d, 1) ~= 0xff then break end
    p = h:seek()
    n = string.byte(d, 2)
    -- DRI, 0xffdd
    if n == 0xdd then
      p = p + 2
    -- RSTn, 0xffd0 - 0xffd7
    elseif (n > 207) and (n < 216) then
      p = p - 2
    else
      p = p + BinToNum(d, 3, 4) - 2
      d = h:read(29)
      if n == 0xe0 then
        if string.byte(d, 5) == 0x00 then
          t = string.sub(d, 1, 4)
          if t == 'JFIF' then
            ar[1] = true
          elseif t == 'JFXX' then
            ar[2] = true
          end
        end
      elseif n == 0xe1 then
        if string.byte(d, 5) == 0x00 then
          if string.sub(d, 1, 4) == 'Exif' then ar[3] = true end
        else
          if string.byte(d, 29) == 0x00 then
            if string.sub(d, 1, 28) == 'http://ns.adobe.com/xap/1.0/' then ar[4] = true end
          end
        end
      elseif n == 0xe2 then
        if string.byte(d, 12) == 0x00 then
          if string.sub(d, 1, 11) == 'ICC_PROFILE' then ar[5] = true end
        end
      elseif n == 0xed then
        if string.byte(d, 14) == 0x00 then
          if string.sub(d, 1, 13) == 'Photoshop 3.0' then ar[6] = true end
        end
      elseif n == 0xee then
        if string.byte(d, 6) == 0x00 then
          if string.sub(d, 1, 5) == 'Adobe' then ar[7] = true end
        end
      elseif n == 0xfe then
        ar[86] = true
      else
        for i = 1, #compres do
          if n == compres[i][2] then
            ar[9] = compres[i][3]
            break
          end
        end
      end
    end
    h:seek('set', p)
  end
  h:close()
  return ar[FieldIndex + 1]
end

function BinToNum(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = r .. string.format('%02x', string.byte(d, i))
  end
  return tonumber('0x' .. r)
end
