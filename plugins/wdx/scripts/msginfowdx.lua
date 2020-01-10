-- msginfowdx.lua (cross-platform)
-- 2020.01.11
--
-- Getting info from headers of saved email messages.
-- Supported formats: *.eml, *.msg and MH mailfile format (Sylpheed and other).
-- See RFC 822, RFC 2822, RFC 5322
--
-- NOTES:
-- 1) Sylpheed and some others don't use file extension, so we will use EXT="*"
--    and check file extension in the beginning of ContentGetValue().
-- 2) The first line should contain any email message field!
-- 3) Decoding Base64: https://github.com/toastdriven/lua-base64
-- 4) TimeZone offset: http://lua-users.org/wiki/TimeZone
--
-- Supported fields: see table "fields".
-- Data type:
--   1 - numeric for "Date as digits";
--   6 - boolean (true or false, i.e. exists or not) for:
--       "Unsubscribe" returns "true" if message has a link for unsubscribe;
--       "DKIM Signature" returns "true" if message has a DKIM signature(s);
--   8 - string.

local fields = {
 {"Date",            "date:",        "UTC date|Sender date", 8},
 {"Date as digits",  "date:",        "UTC date: Y|UTC date: M|UTC date: D|Sender date: Y|Sender date: M|Sender date: D", 1},
 {"From",            "from:",        "full string|email(s) only|name(s) only", 8},
 {"Sender",          "sender:",      "full string|email(s) only|name(s) only", 8},
 {"To",              "to:",          "full string|email(s) only|name(s) only", 8},
 {"Carbon Copy (Cc)", "cc:",         "full string|email(s) only|name(s) only", 8},
 {"Blind Carbon Copy (Bcc)", "bcc:", "full string|email(s) only|name(s) only", 8},
 {"Reply-To",        "reply-to:",    "full string|email(s) only|name(s) only", 8},
 {"Errors-To",       "errors-to:",   "", 8},
 {"Return-Path",     "return-path:", "", 8},
 {"Priority",        "priority:",    "", 8},
 {"Message ID",      "message-id:",  "", 8},
 {"In-Reply-To",     "in-reply-to:", "", 8},
 {"References",      "references:",  "", 8},
 {"Subject",         "subject:",     "", 8},
 {"Comments",        "comments:",    "", 8},
 {"Keywords",        "keywords:",    "", 8},
 {"X-Mailer",        "x-mailer:",    "", 8},
 {"X-Priority",      "x-priority:",  "", 8},
 {"X-Sender",        "x-sender:",    "", 8},
 {"DKIM Signature",  "dkim-signature:",   "", 6},
 {"Unsubscribe",     "list-unsubscribe:", "", 6}
}
local all = {}
local d = {
 {1, 4},
 {6, 7},
 {9, 10}
}
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
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 21 then return nil end
  local at = SysUtils.FileGetAttr(FileName)
  if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
  local e
  if filename ~= FileName then
    e = string.lower(SysUtils.ExtractFileExt(FileName))
    -- e = string.lower(string.match(FileName, '^.+(%..+)$'))
    if (e ~= '.eml') and (e ~= '.msg') and (e ~= '') then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    local c = 1
    all = {}
    for l in h:lines() do
      if string.len(l) == 0 then break end
      if string.match(l, '^[A-Za-z%-]+[\t ]*:') == nil then
        if c == 1 then break end
        if string.find(l, '^%s+', 1, false) ~= nil then
          all[c - 1] = all[c - 1] .. l
        else
          if string.sub(l, 1, 2) == '__' then
            all[c - 1] = all[c - 1] .. string.sub(l, 3, -1)
          else
            if string.sub(all[c - 1], -1, -1) == '=' then
              all[c - 1] = string.sub(all[c - 1], 1, -2) .. l
            end
          end
        end
      else
        table.insert(all, l)
        c = c + 1
      end
    end
    h:close()
    -- Min.: Date and From or Sender or Reply-To
    if #all < 2 then return nil end
    filename = FileName
  end
  local r, t
  r = GetValue(fields[FieldIndex + 1][2])
  if r == nil then
    if (FieldIndex == 20) or (FieldIndex == 21) then return false end
    return nil
  end
  -- date:
  if (FieldIndex == 0) or (FieldIndex == 1) then
    t = FormatDate(r)
    if FieldIndex == 0 then
      return t[UnitIndex + 1]
    else
      if UnitIndex <= 2 then
        return tonumber(string.sub(t[1], d[UnitIndex + 1][1], d[UnitIndex + 1][2]))
      else
        return tonumber(string.sub(t[2], d[UnitIndex - 2][1], d[UnitIndex - 2][2]))
      end
    end
-- from, sender, reply-to, to, cc, bcc
  elseif (FieldIndex >= 2) and (FieldIndex <= 7) then
    t = GetEMAIL(r)
    return t[UnitIndex + 1]
  else
    if (FieldIndex == 20) or (FieldIndex == 21) then return true end
    return r
  end
  return nil
end

function GetValue(s)
  local tf, tv, n1
  for i = 1, #all do
    n1 = string.find(all[i], ':', 1, true)
    tf = string.lower(string.sub(all[i], 1, n1))
    tf = string.gsub(tf, '%s+:', ':')
    if tf == s then
      tv = string.sub(all[i], n1 + 1, -1)
      tv = string.gsub(tv, '^%s+', '')
      tv = string.gsub(tv, '%s+$', '')
      tv = string.gsub(tv, '%s+', ' ')
      if string.find(tv, '=?', 1, true) ~= nil then
        if string.find(tv, '?= =?', 1, true) then
          tv = string.gsub(tv, '%?= =%?', '?==?')
        end
        tv = string.gsub(tv, '(=%?[^%?]+%?[^%?]+%?[^%?]+%?=)',
            function(t) return DecodeValue(t) end)
      end
      break
    end
  end
  return tv
end

function GetEMAIL(s)
  -- full string | email(s) only | name(s) only
  local u = {nil, nil, nil}
  local n1, n2, t
  n1 = string.find(s, ',', 1, true)
  if n1 == nil then
    -- "name <email>" or "email"
    n1 = string.find(s, '<', 1, true)
    if n1 == nil then
      u[1] = s
      u[2] = s
    else
      n2 = string.find(s, '>', n1, true)
      u[2] = TrimSpaces(string.sub(s, n1 + 1, n2 - 1))
      u[3] = TrimSpaces(string.sub(s, 1, n1 - 1))
      u[1] = u[3] .. ' <' .. u[2] .. '>'
    end
  else
    -- "name <email>" or "email" > 1
    for l in string.gmatch(s, '[^,]+') do
      if (l ~= ' ') or (l ~= '') then
        n1 = string.find(l, '<', 1, true)
        if n1 == nil then
          t = TrimSpaces(l)
          u[1] = AddValue(u[1], t)
          u[2] = AddValue(u[2], t)
        else
          n2 = string.find(l, '>', n1, true)
          t1 = TrimSpaces(string.sub(l, 1, n1 - 1))
          t2 = TrimSpaces(string.sub(l, n1 + 1, n2 - 1))
          u[1] = AddValue(u[1], t1 .. ' <' .. t2 .. '>')
          u[2] = AddValue(u[2], t2)
          u[3] = AddValue(u[3], t1)
        end
      end
    end
  end
  return u
end

function AddValue(v, s)
  if v == nil then return s else return v .. ', ' .. s end
end

function TrimSpaces(s)
  s = string.gsub(s, '^%s+', '')
  return string.gsub(s, '%s+$', '')
end

function DecodeValue(s)
  -- =?charset?encoding?encoded-text?=
  local ch, en, tv = string.match(s, '=%?([^%?]+)%?([^%?]+)%?([^%?]+)%?=')
  if ch == nil then return s end
  local t = string.lower(en)
  -- Base64
  if t == 'b' then
    tv = DecodeBase64(tv)
  -- Quoted-Printable
  elseif t == 'q' then
    tv = DecodeQuotedPrintable(tv)
  end
  t = string.lower(ch)
  if t ~= 'utf-8' then tv = EncodeToUTF8(tv, t) end
  return tv
end

function FormatDate(s)
  local m = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}
  local us = {
   ["GMT"] = "+0000",
   ["EDT"] = "-0400",
   ["EST"] = "-0500",
   ["CDT"] = "-0500",
   ["CST"] = "-0600",
   ["MDT"] = "-0600",
   ["MST"] = "-0700",
   ["PDT"] = "-0700",
   ["PST"] = "-0800",
   ["A"] = "+0100",
   ["B"] = "+0200",
   ["C"] = "+0300",
   ["D"] = "+0400",
   ["E"] = "+0500",
   ["F"] = "+0600",
   ["G"] = "+0700",
   ["H"] = "+0800",
   ["I"] = "+0900",
   ["K"] = "+1000",
   ["L"] = "+1100",
   ["M"] = "+1200",
   ["N"] = "-0100",
   ["O"] = "-0200",
   ["P"] = "-0300",
   ["Q"] = "-0400",
   ["R"] = "-0500",
   ["S"] = "-0600",
   ["T"] = "-0700",
   ["U"] = "-0800",
   ["V"] = "-0900",
   ["W"] = "-1000",
   ["X"] = "-1100",
   ["Y"] = "-1200",
   ["Z"] = "+0000"
  }
  local dt = {}
  local r = {"", ""}
  local dtu, u, t1, t2, t3
  dt.day, dt.month, dt.year, dt.hour, dt.min, dt.sec, u = string.match(s, '(%d%d?)%s+([A-Za-z]+)%s+(%d%d%d?%d?)%s+(%d%d):?(%d%d):?(%d?%d?)%s+([^%s]+)')
  -- RFC 822
  if string.len(dt.year) == 2 then
    t1 = tonumber(dt.year)
    if (t1 >= 0) and (t1 <= 49) then
      dt.year = '20' .. dt.year
    elseif (t1 >= 50) and (t1 <= 99) then
      dt.year = '19' .. dt.year
    else
      dt.year = '20' .. dt.year
    end
  end
  for i = 1, #m do
    if dt.month == m[i] then
      dt.month = i
      break
    end
  end
  for k, v in pairs(dt) do
    if (dt[k] == nil) or (dt[k] == '') then dt[k] = 0 else dt[k] = tonumber(v) end
  end
  dtu = os.time(dt)
  -- RFC 822
  if string.find(u, '[A-Z]+.?%d+') then
    -- UT
    u = string.gsub(u, '([^A-Z0-9])(%d%d?):?(%d?%d?)', '%1%2%3')
    if string.len(u) == 4 then
      u = string.sub(u, 1, 1) .. '0' .. string.sub(u, 2, 4)
    end
  elseif string.find(u, '[A-Z]+') then
    if us[u] ~= nil then u = us[u] end
  end
  t1, t2, t3 = string.match(u, '([%-%+])(%d%d)(%d%d)')
  if t1 == nil then t1, t2, t3 = '+', '00', '00' end
  -- UTC
  r[1] = os.date('%Y.%m.%d %H:%M:%S', dtu) .. ' ' .. t1 .. t2 .. t3
  -- sender
  local t = tonumber(t2) * 3600 + tonumber(t3) * 60
  if t1 == '+' then
    r[2] = os.date('%Y.%m.%d %H:%M:%S', dtu + t)
  else
    r[2] = os.date('%Y.%m.%d %H:%M:%S', dtu - t)
  end
--[[
  -- to your local time
  -- Get TimeZone offset
  local ts = os.time()
  local udt = os.date("!*t", ts)
  local ldt = os.date("*t", ts)
  local t = os.difftime(os.time(ldt), os.time(udt))
  r[3] = os.date('%Y.%m.%d %H:%M:%S', dtu + t)
]]
  return r
end

function DecodeBase64(to_decode)
  local index_table = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
  local padded = to_decode:gsub("%s", "")
  local unpadded = padded:gsub("=", "")
  local bit_pattern = ''
  local decoded = ''
  for i = 1, string.len(unpadded) do
    local char = string.sub(to_decode, i, i)
    local offset, _ = string.find(index_table, char)
    if offset == nil then return "Base64: Invalid character found." end
    bit_pattern = bit_pattern .. string.sub(to_binary(offset - 1), 3)
  end
  for i = 1, string.len(bit_pattern), 8 do
    local byte = string.sub(bit_pattern, i, i + 7)
    decoded = decoded .. string.char(tonumber(byte, 2))
  end
  local padding_length = padded:len()-unpadded:len()
  if (padding_length == 1 or padding_length == 2) then
    decoded = decoded:sub(1, -2)
  end
  return decoded
end

function to_binary(integer)
  local remaining = tonumber(integer)
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
  return string.gsub(s, '=([A-F0-9][A-F0-9])',
      function(h) return string.char(tonumber(h,16)) end)
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
