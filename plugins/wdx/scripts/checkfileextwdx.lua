-- checkfileextwdx.lua (cross-platform)
-- 2020.10.30
--[[
Checking that the file extension matches the file type (by the file signatures ("magic numbers"))
and returns some additional info.

List of fields:
- "Description" returns short description;
- "MIME-type" returns MIME-type of file;
- "MIME-type: alias(es)" returns alias if exists (if more than one then comma separated);
- "Ext" returns usual file extension;
- "Bad ext" returns "True", if the file extension is not correspond to the file format.

Notes about "Ext" and "Bad ext"
1) For example, JPEG: "Ext" returns "jpg", because it's usual file extension, but
for checking will use full list: "*.jpeg; *.jpg; *.jpe; *.jfif".

Current list:
- ASF video, Windows Media audio or Windows Media video (*.asf; *.wma; *.wmv)
- BMP image (*.bmp; *.dib; *.rle)
- CHM document (*.chm)
- DjVu document (*.djvu; *.djv)
- GIF image (*.gif)
- JPEG image (*.jpg; *.jpeg; *.jpe; *.jfif)
- MP3 audio (*.mp3)
- PDF document (*.pdf)
- PNG image (*.png)
- RTF document (Rich Text Format) (*.rtf)
- TIFF image (*.tif; *.tiff)
]]

local fields = {
{"Description",          "", 8},
{"MIME-type",            "", 8},
{"MIME-type: alias(es)", "", 8},
{"Ext",                  "", 8},
{"Bad ext",              "", 6}
}
local ar = {}
local filename = ''

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
  if filename ~= FileName then
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000004) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000400) % 2 ~= 0 then return nil end
    ar = {'', '', '', '', false}
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    local d = h:read(32)
    h:close()
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    local d14h = BinToNum(d, 1, 4)
    local d58h = BinToNum(d, 5, 8)
    -- BMP
    if IsBMP(d) == true then
      ar[1] = 'BMP image'
      ar[2] = 'image/bmp'
      ar[3] = 'image/x-bmp, image/x-MS-bmp'
      ar[4] = 'bmp'
      if (e ~= '.bmp') and (e ~= '.dib') and (e ~= '.rle') then ar[5] = true end
    -- CHM
    elseif d14h == 0x49545346 then
      ar[1] = 'CHM document, Microsoft Compiled HTML Help File'
      ar[2] = 'application/vnd.ms-htmlhelp'
      ar[3] = 'application/x-chm'
      ar[4] = 'chm'
      if e ~= '.chm' then ar[5] = true end
    -- DjVu (AT&TFORM)
    elseif (d14h == 0x41542654) and (d58h == 0x464f524d) and (string.sub(d, 13, 15) == 'DJV') then
      ar[1] = 'DjVu document'
      ar[2] = 'image/vnd.djvu'
      ar[4] = 'djvu'
      local t = string.sub(d, 16, 16)
      if t == 'U' then
        ar[3] = 'image/x-djvu, image/x.djvu'
      elseif t == 'M' then
        ar[3] = 'image/x-djvu, image/x.djvu, image/vnd.djvu+multipage'
      end
      if (e ~= '.djvu') and (e ~= '.djv') then ar[5] = true end
    -- GIF
    elseif d14h == 0x47494638 then
      ar[1] = 'GIF image'
      ar[2] = 'image/gif'
      ar[4] = 'gif'
      if e ~= '.gif' then ar[5] = true end
    -- JPEG: ff d8 ff
    elseif (string.byte(d, 1) == 0xff) and (string.byte(d, 2) == 0xd8) and (string.byte(d, 3) == 0xff) then
      ar[1] = 'JPEG image'
      ar[2] = 'image/jpeg'
      ar[4] = 'jpg'
      if (e ~= '.jpg') and (e ~= '.jpeg') and (e ~= '.jpe') and (e ~= '.jfif') then ar[5] = true end
    -- MP3 and MP3 with ID3v2
    elseif (string.sub(d, 1, 3) == 'ID3') or (BinToNum(d, 1, 2) == 0xfffb) then
      ar[1] = 'MP3 audio'
      ar[2] = 'audio/mpeg'
      ar[3] = 'audio/x-mp3, audio/x-mpg, audio/x-mpeg, audio/mp3'
      ar[4] = 'mp3'
      if e ~= '.mp3' then ar[5] = true end
    -- PDF
    elseif string.sub(d, 1, 5) == '%PDF-' then
      ar[1] = 'PDF document'
      ar[2] = 'application/pdf'
      ar[3] = 'application/x-pdf, image/pdf, application/acrobat, application/nappdf'
      ar[4] = 'pdf'
      if e ~= '.pdf' then ar[5] = true end
    -- PNG
    elseif (string.byte(d, 1) == 0x89) and (string.sub(d, 2, 4) == 'PNG') and (d58h == 0x0d0a1a0a) then
      ar[1] = 'PNG image'
      ar[2] = 'image/png'
      ar[4] = 'png'
      if e ~= '.png' then ar[5] = true end
    -- RTF
    elseif string.sub(d, 1, 5) == '{\rtf' then
      ar[1] = 'RTF document (Rich Text Format)'
      ar[2] = 'application/rtf'
      ar[3] = 'text/rtf'
      ar[4] = 'rtf'
      if e ~= '.rtf' then ar[5] = true end
    -- ASF, WMA, WMV
    elseif (d14h == 0x3026b275) and (d58h == 0x8e66cf11) and (BinToNum(d, 9, 12) == 0xa6d900aa) and (BinToNum(d, 13, 16) == 0x0062ce6c) then
      ar[1] = 'ASF video, Windows Media audio or Windows Media video'
      if e == '.asf' then
        ar[1] = 'ASF video'
        ar[2] = 'application/vnd.ms-asf'
        ar[3] = 'video/x-ms-wm, video/x-ms-asf, video/x-ms-asf-plugin'
        ar[4] = 'asf'
      elseif e == '.wma' then
        ar[1] = 'Windows Media audio'
        ar[2] = 'audio/x-ms-wma'
        ar[3] = 'audio/wma'
        ar[4] = 'wma'
      elseif e == '.wmv' then
        ar[1] = 'Windows Media video'
        ar[2] = 'video/x-ms-wmv'
        ar[4] = 'wmv'
      else
        ar[5] = true
      end
    -- TIFF
    elseif (d14h == 0x49492a00) or (d14h == 0x4d4d002a) or (d14h == 0x49492b00) or (d14h == 0x4d4d002b) then
      ar[1] = 'TIFF image'
      ar[2] = 'image/tiff'
      ar[4] = 'tif'
      if (e ~= '.tif') and (e ~= '.tiff') then ar[5] = true end
    end
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
  return nil
end

function BinToNum(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = r .. string.format('%02x', string.byte(d, i))
  end
  return tonumber('0x' .. r)
end

function IsBMP(d)
  if (string.sub(d, 1, 2) == 'BM') and (BinToNum(d, 13, 14) == 0x0000) and (BinToNum(d, 16, 18) == 0x00) then
    if (string.byte(d, 15) == 0x0c) and (BinToNum(d, 23, 24) == 0x0100) and (string.byte(d, 26) == 0x00) then
      return true
    end
    if (BinToNum(d, 27, 28) == 0x0100) and (string.byte(d, 30) == 0x00) then
      return true
    end
  end
  return false
end
