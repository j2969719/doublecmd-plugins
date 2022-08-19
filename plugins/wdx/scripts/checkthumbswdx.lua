-- checkthumbswdx.lua (cross-platform)
-- 2022.08.19
--[[
Getting some information from thumbnails files:
  created by Double Commander (Windows or Linux)
  or the system generator of thumbnails (Linux).

Supported fields: see table "fields".

GetDouble() based on struct.unpack(): Copyright (c) 2015-2020 Iryont <https://github.com/iryont/lua-struct>
URL-decode: http://lua-users.org/wiki/StringRecipes
]]

local fields = {
{"Original file name", 8},
{"Modification date",  8},
{"Obsolete",           6},
{"Not found",          6}
}
local tunpack = table.unpack or _G.unpack
local ar = {}

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2]
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return '(EXT="PNG")|(EXT="JPG")'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  local at = SysUtils.FileGetAttr(FileName)
  if at < 0 then return nil end
  if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
  local h = io.open(FileName, 'rb')
  if h == nil then return nil end
  local tn, tt
  at = h:seek('end')
  h:seek('set', at - 4)
  local d = h:read(4)
  local n = BinToNumLE(d, 1, 4)
  if n < at then
    h:seek('set', n)
    d = h:read(8)
    if (d ~= nil) and (string.byte(d, 1) == 0x00) and (string.byte(d, 2) == 0x00) and (string.sub(d, 3, 8) == '#THUMB') then
      -- Double Commander
      d = h:read(4)
      n = BinToNumLE(d, 1, 4)
      d = h:read(n)
      tn = string.sub(d, 9, -1)
      h:seek('cur', 8)
      d = h:read(8)
      -- Lua 5.3+ only:
      --at = string.unpack('<d', d)
      at = GetDouble(d)
      tt = math.floor((at - 25569) * 86400)
    end
  end
  if tn == nil then
    -- Linux
    h:seek('set', 0)
    d = h:read(8)
    n = BinToNumBE(d, 1, 4)
    at =  BinToNumBE(d, 5, 8)
    if (n == 0x89504e47) and (at == 0x0d0a1a0a) then
      h:seek('cur', 4)
      d = h:read(4)
      if d ~= 'IHDR' then
        h:close()
        return nil
      end
      h:seek('cur', 17)
      local i = 0
      local t
      while i ~= 2 do
        d = h:read(4)
        at = BinToNumBE(d, 1, 4)
        d = h:read(4)
        if d == 'IDAT' then break end
        if d == 'tEXt' then
          d = h:read(at)
          t = string.sub(d, 1, 10)
          if t == 'Thumb::URI' then
            tn = string.sub(d, 19, at)
            i = i + 1
          elseif t == 'Thumb::MTi' then
            tt = tonumber(string.sub(d, 14, at), 10)
            i = i + 1
          end
          h:seek('cur', 4)
        else
          h:seek('cur', at + 4)
        end
      end
    end
  end
  h:close()
  if tn == nil then return nil end
  if string.sub(tn, 2, 3) == ':/' then tn = string.gsub(tn, '/', '\\') end
  ar[1] = string.gsub(tn, "%%(%x%x)", function(h)
    return string.char(tonumber(h, 16))
  end)
  at = CheckAndGetFileTime(ar[1])
  if at == nil then
    ar[3] = true
    ar[4] = true
  else
    if at ~= tt then ar[3] = true else ar[3] = false end
    ar[4] = false
  end
  if tt ~= nil then ar[2] = os.date('%Y.%m.%d %H:%M:%S', tt) else ar[2] = '' end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function BinToNumBE(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = r .. string.format('%02x', string.byte(d, i))
  end
  return tonumber('0x' .. r)
end

function BinToNumLE(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = string.format('%02x', string.byte(d, i)) .. r
  end
  return tonumber('0x' .. r)
end

function GetDouble(x)
  local vars = {}
  local sign = 1
  local mantissa = string.byte(x, 7) % 16
  for i = 6, 1, -1 do
    mantissa = mantissa * (2 ^ 8) + string.byte(x, i)
  end
  if string.byte(x, 8) > 127 then
    sign = -1
  end
  local exponent = (string.byte(x, 8) % 128) * 16 + math.floor(string.byte(x, 7) / 16)
  if exponent == 0 then
    table.insert(vars, 0.0)
  else
    mantissa = (math.ldexp(mantissa, -52) + 1) * sign
    table.insert(vars, math.ldexp(mantissa, exponent - 1023))
  end
  return tunpack(vars)
end

function CheckAndGetFileTime(s)
  local fn = SysUtils.ExtractFileName(s)
  local ft = nil
  local fr = nil
  local h, fd = SysUtils.FindFirst(s)
  if h == nil then
    SysUtils.FindClose(h)
    return nil
  else
    repeat
      if fd.Name == fn then
        ft = fd.Time
        break
      end
      fr, fd = SysUtils.FindNext(h)
    until fr == nil
    SysUtils.FindClose(h)
  end
  return ft
end
