-- vcardinfowdx.lua (cross-platform)
-- 2023.02.07
--[[
vCard Format Specification 2.1, 3.0, 4.0
Some details: https://en.wikipedia.org/wiki/VCard

NOTE: Script reads file content between BEGIN:VCARD and END:VCARD and will continue to work
      only if VERSION property is exists (all vCards must contain the VERSION property).
NOTE: One vCard file - one person! Otherwise only the first one will be read.
      The "Multi" field will return "true" if the file contains more than one entry.

Supported fields: see table "fields".
Lists of units: see "adru" for "Address" fields and third column for other.
Data type:
  6 - boolean (true or false, i.e. exists or not);
  8 - string.
]]

local adru = "Post office box|Extended address|Street|Locality (city etc)|Region (state, province)|Postal code|Country"
local fields = {
{"Version",                         "VERSION",    "", 8},
{"Formatted name",                  "FN",         "", 8},
{"Name",                            "N",          "Last name|First name|Middle name|Name prefix(es)|Name suffix(es)", 8},
{"Gender",                          "GENDER",     "", 8},
{"Nickname",                        "NICKNAME",   "", 8},
{"Photograph",                      "PHOTO",      "", 6},
{"Birthday",                        "BDAY",       "", 8},
{"Address",                         "ADR",        adru, 8},
{"Address: domestic delivery",      "ADR",        adru, 8},
{"Address: international delivery", "ADR",        adru, 8},
{"Address: postal delivery",        "ADR",        adru, 8},
{"Address: parcel delivery",        "ADR",        adru, 8},
{"Address: home",                   "ADR",        adru, 8},
{"Address: work",                   "ADR",        adru, 8},
{"Address: preferred",              "ADR",        adru, 8},
{"Address label",                   "LABEL",      "Default|Domestic delivery|International delivery|Postal delivery|Parcel delivery|Home|Work|Preferred", 8},
{"Telephone",                       "TEL",        "Main|Mobile|Work mobile|Work|Home|Preferred|Work fax|Home fax|Pager|Car phone|BBS|ISDN|PCS|Callback|Telex|TTY/TDD|Other|Assistant", 8},
{"Email",                           "EMAIL",      "Home|Work|Other|Preferred", 8},
{"Email program",                   "MAILER",     "", 8},
{"Time zone",                       "TZ",         "", 8},
{"Global positioning",              "GEO",        "", 8},
{"Title",                           "TITLE",      "", 8},
{"Role or occupation (X.520)",      "ROLE",       "", 8},
{"Logo",                            "LOGO",       "", 6},
{"Organization name",               "ORG",        "", 8},
{"Category",                        "CATEGORIES", "", 8},
{"Note",                            "NOTE",       "", 8},
{"Product ID",                      "PRODID",     "", 8},
{"Last revision",                   "REV",        "", 8},
{"Unique identifier",               "UID",        "", 8},
{"URL",                             "URL",        "", 8},
{"Access classification",           "CLASS",      "", 8},
{"Instant messaging",               "IM",         "AIM|Facebook|Flickr|Gadu-Gadu|Google Hangouts|GroupWise|ICQ|Jabber|Linkedln|MySpace|QQ|Sina Weibo|Skype|Twitter|WhatsApp|Windows Live|Yahoo", 8},
{"Assistant",                       "X-ASSISTANT", "", 8},
{"Multi",                           "", "", 6}
}
local encoding = {
['windows-1250'] = 'cp1250',
['windows-1251'] = 'cp1251',
['windows-1252'] = 'cp1252',
['windows-1253'] = 'cp1253',
['windows-1254'] = 'cp1254',
['windows-1255'] = 'cp1255',
['windows-1256'] = 'cp1256',
['windows-1257'] = 'cp1257',
['windows-1258'] = 'cp1258',
['iso-8859-1'] = 'iso88591',
['iso-8859-2'] = 'iso88592',
['iso-8859-3'] = 'iso88593',
['iso-8859-4'] = 'iso88594',
['iso-8859-5'] = 'iso88595',
['iso-8859-7'] = 'iso88597',
['iso-8859-9'] = 'iso88599',
['iso-8859-10'] = 'iso885910',
['iso-8859-13'] = 'iso885913',
['iso-8859-14'] = 'iso885914',
['iso-8859-15'] = 'iso885915',
['iso-8859-16'] = 'iso885916'
}
local all = {}
local adr = {}
local adrl = {}
local tel = {}
local em = {}
local im = {}
local v
local bm = false
local filename = ''

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][3], fields[FieldIndex + 1][4]
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return '(EXT="VCF")|(EXT="VCARD")'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 34 then return nil end
  local n1, s
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if (e ~= '.vcf') and (e ~= '.vcard') then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    local c, i, j = 1, 1, 0
    local vc = false
    all = {}
    for l in h:lines() do
      if j == 1 then
        if string.sub(l, 1, 11) == 'BEGIN:VCARD' then bm = true end
        break
      end
      if vc == false then
        if string.sub(l, 1, 11) == 'BEGIN:VCARD' then
          vc = true
        else
          if i > 1 then break end
        end
      else
        if string.sub(l, 1, 9) == 'END:VCARD' then
          j = 1
        else
          l = string.gsub(l, '[\r\n]+$', '')
          if string.match(l, '^[A-Z][A-Z%-]*[:;]') == nil then
            while true do
              if string.sub(l, 1, 1) == ' ' then l = string.sub(l, 2, -1) else break end
            end
            n1 = string.len(all[c - 1])
            if string.sub(all[c - 1], n1, n1) == '=' then
              all[c - 1] = string.sub(all[c - 1], 1, n1 - 1) .. l
            else
              all[c - 1] = all[c - 1] .. l
            end
          else
            all[c] = l
            c = c + 1
          end
        end
      end
      i = i + 1
    end
    h:close()
    if vc == false then return nil end
    -- VERSION
    v = nil
    s = GetString('VERSION', 7)
    -- if s == nil then return nil else v = string.sub(s, 9, -1) end
    if s == nil then return 'Invalid vCard!' else v = string.sub(s, 9, -1) end
    CleanTables()
    GetData()
    filename = FileName
  end
  if FieldIndex == 0 then
    return v
  -- ADR
  elseif (FieldIndex >= 7) and (FieldIndex <= 14) then
    if adr[FieldIndex - 6] == nil then
      return nil
    else
      return adr[FieldIndex - 6][UnitIndex + 1]
    end
  -- LABEL
  elseif FieldIndex == 15 then
    return adrl[UnitIndex + 1]
  -- TEL
  elseif FieldIndex == 16 then
    return tel[UnitIndex + 1]
  -- EMAIL
  elseif FieldIndex == 17 then
    return em[UnitIndex + 1]
  -- URL
  elseif FieldIndex == 30 then
    local urls = ''
    for i = 1, #all do
      s = string.sub(all[i], 1, 4)
      if (s == 'URL:') or (s == 'URL;') then
        n1 = string.find(all[i], ':', 1, true)
        s = string.sub(s, n1 + 1, -1)
        if urls == '' then urls = s else urls = urls .. ', ' .. s end
      end
    end
    return urls
  -- IM
  elseif FieldIndex == 32 then
    return im[UnitIndex + 1]
  end
  -- Other fields
  s = GetString(fields[FieldIndex + 1][2], string.len(fields[FieldIndex + 1][2]))
  if s == nil then
    -- PHOTO & LOGO
    if (FieldIndex == 5) or (FieldIndex == 23) then return false end
    return nil
  end
  -- FN
  if FieldIndex == 1 then
    return GetValue(s, 'FN', 2)
  -- N
  elseif FieldIndex == 2 then
    if string.sub(s, 1, 2) == 'N:' then
      s = string.sub(s, 3, -1)
    else
      s = DecodeValue(s)
    end
    local ln, fn, mn, np, ns = string.match(s, '([^;]*);?([^;]*);?([^;]*);?([^;]*);?([^;]*)')
    if ln == nil then return nil end
    if UnitIndex == 0 then
      return ln
    elseif UnitIndex == 1 then
      return fn
    elseif UnitIndex == 2 then
      return mn
    elseif UnitIndex == 3 then
      return np
    elseif UnitIndex == 4 then
      return ns
    else
      return nil
    end
  -- GENDER
  elseif FieldIndex == 3 then
    n1 = string.find(s, ':', 1, true)
    s = string.sub(s, n1 + 1, -1)
    if s == 'M' then
      return 'Male'
    elseif s == 'F' then
      return 'Female'
    elseif s == 'O' then
      return 'Other'
    elseif s == 'N' then
      return 'None or not applicable'
    elseif s == 'U' then
      return 'Unknown'
    end
  -- NICKNAME
  elseif FieldIndex == 4 then
    return GetValue(s, 'NICKNAME', 8)
  -- PHOTO
  elseif FieldIndex == 5 then
    return true
  -- BDAY
  elseif FieldIndex == 6 then
    n1 = string.find(s, ':', 1, true)
    s = string.sub(s, n1 + 1, -1)
    if string.sub(s, 1, 2) == '--' then
      return string.gsub(s, '^%-%-%-?(%d%d)%-?(%d%d)', '-----%1-%2', 1)
    else
      return FormatDate(s)
    end
  -- MAILER
  elseif FieldIndex == 18 then
    return string.sub(s, 8, -1)
  -- TZ
  elseif FieldIndex == 19 then
    -- UTC offset would be "+01:00", "+0100", or simply "+01"
    s = DecodeValue(s)
    if string.find(s, '%d%d:%d%d') == nil then
      n1 = 0
      s, n1 = string.gsub(s, '(%d%d)(%d%d)', '%1:%2', 1)
      if n1 ~= 1 then
        s = string.gsub(s, '([^%d]*)(%d%d)([^%d]*)', '%1%2:00%3', 1)
      end
    end
    return s
  -- GEO
  elseif FieldIndex == 20 then
    n1 = string.find(s, ':', 1, true)
    return string.sub(s, n1 + 1, -1)
  -- TITLE
  elseif FieldIndex == 21 then
    return GetValue(s, 'TITLE', 5)
  -- ROLE
  elseif FieldIndex == 22 then
    return GetValue(s, 'ROLE', 4)
  -- LOGO
  elseif FieldIndex == 23 then
    return true
  -- ORG
  elseif FieldIndex == 24 then
    return GetValue(s, 'ORG', 3)
  -- CATEGORIES
  elseif FieldIndex == 25 then
    return GetValue(s, 'CATEGORIES', 10)
  -- NOTE
  elseif FieldIndex == 26 then
    return GetValue(s, 'NOTE', 4)
  -- PRODID
  elseif FieldIndex == 27 then
    return string.sub(s, 8, -1)
  -- REV
  elseif FieldIndex == 28 then
    return FormatDate(string.sub(s, 5, -1))
  -- UID
  elseif FieldIndex == 29 then
    n1 = string.find(s, ':', 1, true)
    return string.sub(s, n1 + 1, -1)
  -- CLASS
  elseif FieldIndex == 31 then
    n1 = string.find(s, ':', 1, true)
    return string.sub(s, n1 + 1, -1)
  -- X-ASSISTANT
  elseif FieldIndex == 33 then
    return GetValue(s, 'X-ASSISTANT', 11)
  -- Multi
  elseif FieldIndex == 34 then
    return bm
  end
  return nil
end

function CleanTables()
  for i = 1, 8 do adr[i] = nil end
  for i = 1, 8 do adrl[i] = nil end
  for i = 1, 18 do tel[i] = nil end
  for i = 1, 4 do em[i] = nil end
  for i = 1, 17 do im[i] = nil end
end

function GetString(p, n)
  local t
  for i = 1, #all do
    t = string.sub(all[i], 1, n + 1)
    if (t == p .. ':') or (t == p .. ';') then return all[i] end
  end
  return nil
end

function GetValue(s, p, n)
  if string.sub(s, 1, n + 1) == p .. ':' then
    s = string.sub(s, n + 2, -1)
  else
    s = DecodeValue(s)
  end
  return EscapedChar(s)
end

function GetData()
  local t
  for i = 1, #all do
    t = string.match(all[i], '^([A-Z][A-Z%-]*)[:;]')
    if t == 'ADR' then
      GetADR(all[i], i)
    elseif t == 'LABEL' then
      GetLABEL(all[i])
    elseif t == 'TEL' then
      GetTEL(all[i])
    elseif t == 'EMAIL' then
      GetEMAIL(all[i])
    else
      if string.sub(t, 1, 2) == 'X-' then GetIM(all[i], t) end
    end
  end
end

function GetADR(s, i)
  local tp, tv, n, l
  n = string.find(s, ':', 1, true)
  tp = string.lower(string.sub(s, 4, n - 1))
  tv = DecodeValue(s)
  tv = EscapedChar(tv)
  if tp == '' then
    adr[1] = GetADRUnit(tv)
  else
    if (v ~= nil) and (v ~= '2.1') and (v ~= '3.0') then
      local n1, n2 = string.find(tp, 'label="', 1, true)
      if n1 ~= nil then
        n1 = string.find(tp, '"', n2 + 1, true)
        l = string.sub(all[i], n2 + 5, n1 + 3)
        if string.find(tp, 'encoding=quoted-printable', 1, true) ~= nil then
          l = DecodeQuotedPrintable(l)
        end
        local t = string.match(tp, 'charset=[^:;]+')
        if t ~= nil then
          if t ~= 'utf-8' then l = EncodeToUTF8(l, t) end
        end
        l = EscapedChar(l)
      end
    end
    if string.find(tp, 'pref', 1, true) then
      adr[8] = GetADRUnit(tv)
      if l ~= nil then adrl[8] = l end
    end
    if string.find(tp, 'dom', 1, true) then
      adr[2] = GetADRUnit(tv)
      if l ~= nil then adrl[2] = l end
    elseif string.find(tp, 'intl', 1, true) then
      adr[3] = GetADRUnit(tv)
      if l ~= nil then adrl[3] = l end
    elseif string.find(tp, 'postal', 1, true) then
      adr[4] = GetADRUnit(tv)
      if l ~= nil then adrl[4] = l end
    elseif string.find(tp, 'parcel', 1, true) then
      adr[5] = GetADRUnit(tv)
      if l ~= nil then adrl[5] = l end
    elseif string.find(tp, 'home', 1, true) then
      adr[6] = GetADRUnit(tv)
      if l ~= nil then adrl[6] = l end
    elseif string.find(tp, 'work', 1, true) then
      adr[7] = GetADRUnit(tv)
      if l ~= nil then adrl[7] = l end
    else
      adr[1] = GetADRUnit(tv)
      if l ~= nil then adrl[1] = l end
    end
  end
end

function GetADRUnit(s)
  local uni = {nil, nil, nil, nil, nil, nil, nil}
  local po, ed, sa, lo, re, pc, co = string.match(s, '([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*);([^;]*)')
  if po == nil then return uni end
  if po ~= '' then uni[1] = po end
  if ed ~= '' then uni[2] = ed end
  if sa ~= '' then uni[3] = sa end
  if lo ~= '' then uni[4] = lo end
  if re ~= '' then uni[5] = re end
  if pc ~= '' then uni[6] = pc end
  if co ~= '' then uni[7] = co end
  return uni
end

function GetLABEL(s)
  local tp, tv, n
  n = string.find(s, ':', 1, true)
  tp = string.lower(string.sub(s, 6, n - 1))
  tv = DecodeValue(s)
  tv = EscapedChar(tv)
  if tp == '' then
    adrl[1] = tv
  else
    if string.find(tp, 'pref', 1, true) then adrl[8] = tv end
    if string.find(tp, 'dom', 1, true) then
      adrl[2] = tv
    elseif string.find(tp, 'intl', 1, true) then
      adrl[3] = tv
    elseif string.find(tp, 'postal', 1, true) then
      adrl[4] = tv
    elseif string.find(tp, 'parcel', 1, true) then
      adrl[5] = tv
    elseif string.find(tp, 'home', 1, true) then
      adrl[6] = tv
    elseif string.find(tp, 'work', 1, true) then
      adrl[7] = tv
    else
      adrl[1] = tv
    end
  end
end

function GetTEL(s)
  local tp, tv, n
  n = string.find(s, ':', 1, true)
  tp = string.lower(string.sub(s, 4, n - 1))
  tv = string.sub(s, n + 1, -1)
  if tp == '' then
    tel[17] = AddValue(tel[17], tv)
  else
    if string.find(tp, 'pref', 1, true) then tel[6] = AddValue(tel[6], tv) end
    if string.find(tp, 'main', 1, true) then
      tel[1] = AddValue(tel[1], tv)
    elseif string.find(tp, 'cell', 1, true) then
      if string.find(tp, 'work', 1, true) then
        tel[3] = AddValue(tel[3], tv)
      else
        tel[2] = AddValue(tel[2], tv)
      end
    elseif string.find(tp, 'work', 1, true) then
      if string.find(tp, 'fax', 1, true) then
        tel[7] = AddValue(tel[7], tv)
      else
        tel[4] = AddValue(tel[4], tv)
      end
    elseif string.find(tp, 'home', 1, true) then
      if string.find(tp, 'fax', 1, true) then
        tel[8] = AddValue(tel[8], tv)
      else
        tel[5] = AddValue(tel[5], tv)
      end
    elseif string.find(tp, 'pager', 1, true) then
      tel[9] = AddValue(tel[9], tv)
    elseif string.find(tp, 'car', 1, true) then
      tel[10] = AddValue(tel[10], tv)
    elseif string.find(tp, 'bbs', 1, true) then
      tel[11] = AddValue(tel[11], tv)
    elseif string.find(tp, 'isdn', 1, true) then
      tel[12] = AddValue(tel[12], tv)
    elseif string.find(tp, 'pcs', 1, true) then
      tel[13] = AddValue(tel[13], tv)
    elseif string.find(tp, 'callback', 1, true) then
      tel[14] = AddValue(tel[14], tv)
    elseif string.find(tp, 'tlx', 1, true) then
      tel[15] = AddValue(tel[15], tv)
    elseif string.find(tp, 'tty-tdd', 1, true) then
      tel[16] = AddValue(tel[16], tv)
    elseif string.find(tp, 'assistant', 1, true) then
      tel[18] = AddValue(tel[18], tv)
    else
      tel[17] = AddValue(tel[17], tv)
    end
  end
end

function GetEMAIL(s)
  local tp, tv, n
  n = string.find(s, ':', 1, true)
  tp = string.lower(string.sub(s, 6, n - 1))
  tv = string.sub(s, n + 1, -1)
  if tp == '' then
    em[3] = AddValue(em[3], tv)
  else
    if string.find(tp, 'pref', 1, true) then em[4] = AddValue(em[4], tv) end
    if string.find(tp, 'home', 1, true) then
      em[1] = AddValue(em[1], tv)
    elseif string.find(tp, 'work', 1, true) then
      em[2] = AddValue(em[2], tv)
    else
      em[3] = AddValue(em[3], tv)
    end
  end
end

function GetIM(s, t)
  local tv = DecodeValue(s)
  if t == 'X-AIM' then
    im[1] = AddValue(im[1], tv)
  elseif t == 'X-FACEBOOK' then
    im[2] = AddValue(im[2], tv)
  elseif t == 'X-GADUGADU' then
    im[4] = AddValue(im[4], tv)
  elseif (t == 'X-GOOGLE-TALK') or t == ('X-GTALK') then
    im[5] = AddValue(im[5], tv)
  elseif t == 'X-GROUPWISE' then
    im[6] = AddValue(im[6], tv)
  elseif t == 'X-ICQ' then
    im[7] = AddValue(im[7], tv)
  elseif t == 'X-JABBER' then
    im[8] = AddValue(im[8], tv)
  elseif t == 'X-QQ' then
    im[11] = AddValue(im[11], tv)
  elseif (t == 'X-SKYPE') or t == ('X-SKYPE-USERNAME') then
    im[13] = AddValue(im[13], tv)
  elseif t == 'X-TWITTER' then
    im[14] = AddValue(im[14], tv)
  elseif t == 'X-WHATSAPP' then
    im[15] = AddValue(im[15], tv)
  elseif t == 'X-MSN' then
    im[16] = AddValue(im[16], tv)
  elseif t == 'X-YAHOO' then
    im[17] = AddValue(im[17], tv)
  else
    -- Apple Inc.
    if t == 'X-SOCIALPROFILE' then
      n = string.find(s, ':', 1, true)
      tp = string.sub(s, 15, n)
      if string.find(tp, 'facebook', 1, true) then
        im[2] = AddValue(im[2], tv)
      elseif string.find(tp, 'flickr', 1, true) then
        im[3] = AddValue(im[3], tv)
      elseif string.find(tp, 'linkedin', 1, true) then
        im[9] = AddValue(im[9], tv)
      elseif string.find(tp, 'myspace', 1, true) then
        im[10] = AddValue(im[10], tv)
      elseif string.find(tp, 'sinaweibo', 1, true) then
        im[12] = AddValue(im[12], tv)
      elseif string.find(tp, 'twitter', 1, true) then
        im[14] = AddValue(im[14], tv)
      end
    end
  end
end

function AddValue(r, s)
  if r == nil then return s else return r .. ', ' .. s end
end

function DecodeValue(s)
  local tv, t, n
  n = string.find(s, ':', 1, true)
  tv = string.sub(s, n + 1, -1)
  if string.find(s, 'ENCODING=QUOTED-PRINTABLE', 1, true) ~= nil then
    tv = DecodeQuotedPrintable(tv)
  end
  t = string.match(s, 'CHARSET=[^:;]+')
  if t ~= nil then
    if t ~= 'UTF-8' then tv = EncodeToUTF8(tv, string.lower(t)) end
  end
  return tv
end

function FormatDate(s)
  if string.sub(s, 5, 5) == '-' then
    s = string.gsub(s, 'T', ' ', 1)
    s = string.gsub(s, 'Z', '', 1)
  else
    local n = string.len(s)
    if n == 8 then
      s = string.gsub(s, '(%d%d%d%d)(%d%d)(%d%d)', '%1-%2-%3')
    else
      if string.sub(s, n, n) == 'Z' then
        s = string.gsub(s, '(%d%d%d%d)(%d%d)(%d%d)T(%d%d)(%d%d)(%d%d)Z', '%1-%2-%3 %4:%5:%6')
      else
        s = string.gsub(s, '(%d%d%d%d)(%d%d)(%d%d)T(%d%d)(%d%d)(%d%d)%-?(%d%d)(%d%d)', '%1-%2-%3 %4:%5:%6-%7:%8')
      end
    end
  end
  return s
end

function EscapedChar(s)
  s = string.gsub(s, '\\\\', '\\')
  s = string.gsub(s, '\\n', '\n')
  s = string.gsub(s, '\\N', '\n')
  s = string.gsub(s, '\\;', ';')
  s = string.gsub(s, '\\,', ',')
  return s
end

function DecodeQuotedPrintable(s)
  return string.gsub(s, '=([A-F0-9][A-F0-9])',
      function(h) return string.char(tonumber(h,16)) end)
end

function EncodeToUTF8(s, e)
  local t = encoding[e]
  if t ~= nil then return LazUtf8.ConvertEncoding(s, t, 'utf8') end
  return s
end
