-- zsyncwdx.lua (cross-platform)
-- 2021.12.24
--[[
Getting some information from zsync-files.
]]

local fields = {
{"Version",      "",               8, "zsync"},
{"Filename",     "",               8, "Filename"},
{"Date",         "",               8, "MTime"},
{"Block size",   "",               1, "Blocksize"},
{"Size",         "B|KB|MB|GB",     3, "Length"},
{"Size, auto B/KB/MB/GB", "",      8, ""},
{"Hash lengths", "",               8, "Hash%-Lengths"},
{"SHA-1",        "",               8, "SHA%-1"},
{"URL",          "URL1|URL2|URL3", 8, ""}
}
local am = {
["Jan"] = "01",
["Feb"] = "02",
["Mar"] = "03",
["Apr"] = "04",
["May"] = "05",
["Jun"] = "06",
["Jul"] = "07",
["Aug"] = "08",
["Sep"] = "09",
["Oct"] = "10",
["Nov"] = "11",
["Dec"] = "12"
}
local ar = {}
local filename = ""

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
  return 'EXT="ZSYNC"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000004) % 2 ~= 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    for i = 1, #ar do ar[i] = '' end
    if GetData(FileName) == nil then return nil end
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    if fields[FieldIndex + 1][2] == '' then
      return ar[FieldIndex + 1]
    else
      local tmp = ar[FieldIndex + 1][UnitIndex + 1]
      if tmp == '' then return nil else return tmp end
    end
  end
  return nil
end

function GetData(FileName)
  local h = io.open(FileName, 'rb')
  if h == nil then return nil end
  local fc = h:read(6)
  if (fc == nil) or (fc ~= "zsync:") then
    h:close()
    return nil
  end
  local tmp = h:read(250)
  fc = fc .. tmp
  local n = string.find(fc, '\n\n', 1, true)
  if n == nil then
    while true do
      tmp = h:read(256)
      if tmp == nil then
        h:close()
        return nil
      end
      fc = fc .. tmp
      n = string.find(fc, '\n\n', string.len(fc) - 257, true)
      if n ~= nil then break end
    end
  end
  h:close()
  for i = 1, #fields do
   if fields[i][4] ~= '' then
      tmp = string.match(fc, fields[i][4] .. ': ([^\n]+)\n')
      if tmp == nil then
        ar[i] = ''
      else
        if fields[i][3] < 4 then
          ar[i] = tonumber(tmp)
        else
          ar[i] = tmp
        end
      end
    end
  end
  -- Date
  if ar[3] ~= '' then
    local d, m, y, t = string.match(ar[3], '(%d+) ([A-Za-z]+) (%d%d%d%d) ([^ ]+)')
    ar[3] = y .. '.' .. am[m] .. '.' .. d .. ' ' .. t
  end
  -- Size
  tmp = ar[5]
  ar[5] = {tmp, '', '', ''}
  for i = 2, 4 do
    tmp = string.format("%.1f", ar[5][1] / (1024 ^ (i - 1)))
    ar[5][i] = tonumber(tmp)
  end
  -- Auto unit
  tmp = ar[5][1] / 1073741824
  if tmp < 1 then
    tmp = ar[5][1] / 1048576
    if tmp < 1 then
      tmp = ar[5][1] / 1024
      if tmp < 1 then
        ar[6] = tmp .. ' B'
      else
        ar[6] = string.format("%.1f", tmp) .. ' KB'
      end
    else
      ar[6] = string.format("%.1f", tmp) .. ' MB'
    end
  else
    ar[6] = string.format("%.1f", tmp) .. ' GB'
  end
  -- URLs
  ar[9] = {'', '', ''}
  n = 1
  for s in string.gmatch(fc, 'URL: ([^\n]+)\n') do
    if string.sub(s, 1, 7) ~= 'zsync: ' then ar[9][n] = s end
    if n == 3 then break end
    n = n + 1
  end
  return 1
end
