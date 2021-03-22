-- icowdx.lua (cross-platform)
-- 2021.02.22
--[[
Getting some info from Windows icons (ICO images):
- the number of images;
- a list of available sizes;
- plugin can check that the specific size is available (see "aun" below).
]]

local aun = {"16x16", "24x24", "32x32", "48x48", "64x64", "96x96", "128x128", "256x256"}
local ar = {}
local filename = ''

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Number of images", "", 1
  elseif FieldIndex == 1 then
    return "List of sizes", "", 8
  elseif FieldIndex == 2 then
    return "Has", table.concat(aun, '|'), 6
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="ICO"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 2 then return nil end
  local t
  if filename ~= FileName then
    t = string.lower(SysUtils.ExtractFileExt(FileName))
    if t ~= '.ico' then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000004) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000400) % 2 ~= 0 then return nil end
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    local d = h:read(4)
    if BinToNum(d, 1, 4) ~= 0x00010000 then
      h:close()
      return nil
    end
    ar = {0, ''}
    d = h:read(2)
    ar[1] = BinToNum(d, 1, 2)
    if ar[1] > 0 then
      local c = ar[1]
      local al1 = {}
      local al2 = {}
      local al3 = {}
      local w, n
      while true do
        if c == 0 then break end
        -- width
        d = h:read(1)
        n = string.byte(d)
        if n == 0x00 then w = 256 else w = tonumber(n, 10) end
        -- height
        d = h:read(1)
        n = string.byte(d)
        if n == 0x00 then t = w .. 'x256' else t = w .. 'x' .. tonumber(n, 10) end
        if w > 99 then
          al3[#al3 + 1] = t
        elseif w > 9 then
          al2[#al2 + 1] = t
        else
          al1[#al1 + 1] = t
        end
        h:seek('cur', 14)
        c = c - 1
      end
      h:close()
      local sr = '; '
      if #al1 > 0 then
        table.sort(al1)
        sr = sr .. al1[1] .. '; '
        for i = 2, #al1 do
          if al1[i] ~= al1[i - 1] then sr = sr .. al1[i] .. '; ' end
        end
      end
      if #al2 > 0 then
        table.sort(al2)
        sr = sr .. al2[1] .. '; '
        for i = 2, #al2 do
          if al2[i] ~= al2[i - 1] then sr = sr .. al2[i] .. '; ' end
        end
      end
      if #al3 > 0 then
        table.sort(al3)
        sr = sr .. al3[1] .. '; '
        for i = 2, #al3 do
          if al3[i] ~= al3[i - 1] then sr = sr .. al3[i] .. '; ' end
        end
      end
      ar[2] = sr
    else
      return nil
    end
    filename = FileName
  end
  if FieldIndex == 0 then
    return ar[1]
  elseif FieldIndex == 1 then
    if string.len(ar[2]) > 4 then return string.sub(ar[2], 3, -3) end
  elseif FieldIndex == 2 then
    if aun[UnitIndex + 1] ~= nil then
      t = string.find(ar[2], '; ' .. aun[UnitIndex + 1] .. ';', 1, true)
      if t == nil then return false else return true end 
    end
  end
  return nil
end

function BinToNum(d, n1, n2)
  local r = ''
  for i = n1, n2 do
    r = string.format('%02x', string.byte(d, i)) .. r
  end
  return tonumber('0x' .. r)
end
