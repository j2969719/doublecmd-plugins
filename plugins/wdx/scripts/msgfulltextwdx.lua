-- msgfulltextwdx.lua (cross-platform)
-- 2025.04.07
--[[
For Find files with plugins only!

Returns full text of saved email messages.
Message body only ("Content-Type: text/plain" or "Content-Type: text/html"), for header use msginfowdx.lua).

Supported formats: *.eml, *.msg and MH format (Sylpheed, Claws Mail and other).
See RFC 822, 2822, 5322 and 2045, 2046, 2047

NOTES:
1) Sylpheed and some others don't use file extension, so we will use EXT="*"
   and check file extension in the beginning of ContentGetValue().
2) The first line should contain any email message field!
3) Decoding Base64: https://github.com/toastdriven/lua-base64
   with small modifications.
]]

local sbody
local filename = ''
-- Optimisation
local tonum = tonumber
local str_char = string.char
local str_find = string.find
local str_gsub = string.gsub
local str_len = string.len
local str_lower = string.lower
local str_match = string.match
local str_sub = string.sub

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Full text (body)", "", 9
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
  if FieldIndex ~= 0 then return nil end
  if UnitIndex ~= 0 then return nil end
  if filename ~= FileName then
    local e = str_lower(SysUtils.ExtractFileExt(FileName))
    if (e ~= '.eml') and (e ~= '.msg') and (e ~= '') then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    local c, hd, hc, bc = 0, 0, 1, 1
    local ctv, cte, s
    local head = {}
    local body = {}
    for l in h:lines() do
      if c == 0 then
        if str_match(l, '^([A-Za-z][A-Za-z0-9%-]+)[\t ]*:') == nil then
          break
        else
          head[hc] = l
          hc = hc + 1
        end
      else
        if hd == 0 then
          if str_len(l) == 0 then
            hd = 1
          else
            head[hc] = l
            hc = hc + 1
          end
        else
          body[bc] =  l
          bc = bc + 1
        end
      end
      c = 2
    end
    h:close()
    for i = 1, #head do
      if (ctv ~= nil) and (cte ~= nil) then break end
      s = str_match(head[i], '^([A-Za-z%-]+)[\t ]*:')
      if s ~= nil then
        s = str_lower(s)
        if s == 'content-type' then
          ctv = head[i]
          -- The value may be long
          if head[i + 1] ~= nil then
            for j = i + 1, #head do
              if str_find(head[j], '^%s+', 1, false) ~= nil then
                ctv = ctv .. head[j]
              else
                break
              end
            end
          end
        else
          if s == 'content-transfer-encoding' then cte = head[i] end
        end
      end
    end
    if str_find(ctv, 'multipart/', 1, true) ~= nil then
      GetMultiPart(body, ctv)
    else
      if str_find(ctv, 'text/plain', 1, true) == nil then
        if str_find(ctv, 'text/html', 1, true) == nil then return nil end
      end
      if cte == nil then cte = 'Content-Transfer-Encoding: 8bit' end
      sbody = GetSingle(body, ctv, cte)
    end
    filename = FileName
  end
  if sbody == '' then return nil else return sbody end
end

function GetSingle(bd, v, e)
  local sbd = ''
  local d = 1
  for i = 1, #bd do
    if bd[i] ~= '' then
      d = i
      break
    end
  end
  local enc = str_match(e, ':%s*([A-Za-z0-9%-]+)')
  enc = str_lower(enc)
  if enc == 'base64' then
    for i = d, #bd do sbd = sbd .. bd[i] end
    if sbd ~= '' then sbd = DecodeBase64(sbd) end
  elseif enc == 'quoted-printable' then
    sbd = bd[d]
    for i = d + 1, #bd do
      if str_sub(bd[i - 1], -1, -1) == '=' then
        sbd = str_sub(sbd, 1, -2) .. bd[i]
      else
        sbd = sbd .. '\n' .. bd[i]
      end
    end
    if sbd ~= '' then sbd = DecodeQuotedPrintable(sbd) end
  else
    for i = d, #bd do sbd = sbd .. '\n' .. bd[i] end
  end
  if sbd ~= '' then
    local cset = str_match(v, 'charset%s*=%s*"?([A-Za-z0-9%-]+)"?')
    if cset ~= nil then
      cset = str_lower(cset)
      if cset ~= 'utf-8' then sbd = EncodeToUTF8(sbd, cset) end
    end
  end
  return sbd
end

function GetMultiPart(bd, v)
  if str_find(v, 'multipart/alternative', 1, false) ~= nil then
    GetMultiPartAlt(bd, v)
  else
    -- multipart: "related", "signed" and other "mixed"
    local t = {}
    local c, d, g = 1, 1, 0
    local hs, he
    local bo = str_match(v, 'boundary="?([^"]+)"?')
    -- Get first part (other parts are usualy attached file(s) or signature (PGP, etc)
    for i = 1, #bd do
      if g == 0 then
        if bd[i] == '--' .. bo then
          hs = i + 1
          g = 1
        end
      else
        if (bd[i] == '--' .. bo) or (bd[i] == '--' .. bo .. '--') then
          he = i - 1
          break
        end
        t[c] = bd[i]
        c = c + 1
      end
    end
    if he == nil then he = #bd end
    for i = hs, he do
      if bd[i] ~= '' then
        hs = i
        break
      end
    end
    d = he - hs
    if d < 3 then return nil end
    local tv, te, s
    for i = hs, he do
      if bd[i] == '' then
        hs = i + 1
        break
      end
      s = str_match(bd[i], '^([A-Za-z%-]+)[\t ]*:')
      if s ~= nil then
        s = str_lower(s)
        if s == 'content-type' then
          tv = bd[i]
          if bd[i + 1] ~= nil then
            for j = i + 1, #bd do
              if str_find(bd[j], '^%s+', 1, false) ~= nil then
                tv = tv .. bd[j]
              else
                break
              end
            end
          end
        else
          if s == 'content-transfer-encoding' then te = bd[i] end
        end
      end
    end
    for i = hs, he do
      if bd[i] ~= '' then
        hs = i
        break
      end
    end
    d = he - hs
    if d < 1 then return nil end
    if str_find(tv, 'multipart/', 1, true) ~= nil then
      g = 2
    else
      if str_find(tv, 'text/plain', 1, true) == nil then
        if str_find(tv, 'text/html', 1, true) == nil then return nil end
      end
      if te == nil then te = 'Content-Transfer-Encoding: 8bit' end
      g = 3
    end
   if g > 1 then
      t = {}
      c = 1
      for i = hs, he do
        t[c] = bd[i]
        c = c + 1
      end
      if g == 2 then
        GetMultiPart(t, tv)
      elseif g == 3 then
        sbody = GetSingle(t, tv, te)
      end
    end
  end
end

function GetMultiPartAlt(b, v)
  sbody = ''
  local t = {}
  local c, d, i, out = 1, 1, 1, 0
  local bo = str_match(v, 'boundary="?([^"]+)"?')
  while true do
    if (out == 1) or (c >= #b) then break end
    for j = c, #b do
      if b[j] == '--' .. bo then
        t = {}
        for k = j + 1, #b do
          if b[k] == '--' .. bo .. '--' then
            out = 1
            break
          elseif b[k] == '--' .. bo then
            c = k
            break
          end
          t[i] = b[k]
          i = i + 1
        end
        break
      end
    end
    if #t ~= 0 then GetText(t) end
    i = 1
  end
end

function GetText(b)
  local c, d = 1, 1
  local bh = {}
  local bhc = 1
  for i = 1, #b do
    if b[i] ~= '' then
      d = i
      break
    end
  end
  for i = d, #b do
    if b[i] == '' then
      c = i + 1
      break
    end
    bh[bhc] = b[i]
    bhc = bhc + 1
  end
  if #bh == 0 then return nil end
  local tv, te, s
  for i = 1, #bh do
    if (tv ~= nil) and (te ~= nil) then break end
    s = str_match(bh[i], '^([A-Za-z%-]+)[\t ]*:')
    if s ~= nil then
      s = str_lower(s)
      if s == 'content-type' then
        tv = bh[i]
        if bh[i + 1] ~= nil then
          for j = i + 1, #bh do
            if str_find(bh[j], '^%s+', 1, false) ~= nil then
              tv = tv .. bh[j]
            else
              break
            end
          end
        end
      else
        if s == 'content-transfer-encoding' then te = bh[i] end
      end
    end
  end
  if str_find(tv, 'text/plain', 1, true) == nil then
    if str_find(tv, 'text/html', 1, true) == nil then return nil end
  end
  d = c
  for i = c, #b do
    if b[i] ~= '' then
      d = i
      break
    end
  end
  c = 1
  bb = {}
  for i = d, #b do
    bb[c] = b[i]
    c = c + 1
  end
  if te == nil then te = 'Content-Transfer-Encoding: 8bit' end
  local t = GetSingle(bb, tv, te)
  sbody = sbody .. '\n' .. t
end

function DecodeBase64(to_decode)
  local index_table = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
  local padded
  if str_find(to_decode, "%s", 1, false) then
    padded = str_gsub(to_decode, "%s", "")
  else
    padded = to_decode
  end
  local unpadded
  if str_find(to_decode, "=", 1, true) then
    unpadded = str_gsub(padded, "=", "")
  else
    unpadded = padded
  end
  local bit_pattern = ''
  local decoded = ''
  for i = 1, str_len(unpadded) do
    local char = str_sub(to_decode, i, i)
    local offset, _ = str_find(index_table, char)
    if offset == nil then return "Base64: Invalid character found." end
    bit_pattern = bit_pattern .. str_sub(to_binary(offset - 1), 3)
  end
  for i = 1, str_len(bit_pattern), 8 do
    local byte = str_sub(bit_pattern, i, i + 7)
    decoded = decoded .. str_char(tonum(byte, 2))
  end
  local padding_length = str_len(padded) - str_len(unpadded)
  if (padding_length == 1 or padding_length == 2) then
    decoded = str_sub(decoded, 1, -2)
  end
  return decoded
end

function to_binary(integer)
  local remaining = tonum(integer)
  local bin_bits = ''
  for i = 7, 0, -1 do
    local current_power = 2 ^ i
    if remaining >= current_power then
      bin_bits = bin_bits .. '1'
      remaining = remaining - current_power
    else
      bin_bits = bin_bits .. '0'
    end
  end
  return bin_bits
end

function DecodeQuotedPrintable(s)
  return str_gsub(s, '=([A-F0-9][A-F0-9])',
      function(h) return str_char(tonum(h,16)) end)
end

function EncodeToUTF8(s, e)
  local l = {
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
   ['iso-8859-15'] = 'iso885915',
   ['koi8-r'] = 'koi8',
   ['mac'] = 'macintosh',
   ['macintosh'] = 'macintosh'
  }
  if l[e] ~= nil then return LazUtf8.ConvertEncoding(s, l[e], 'utf8') end
  return s
end
