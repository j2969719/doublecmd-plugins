-- marker.lua (cross-platform)
-- 2022.12.04

local params = {...}

local dbn, dbne, h, err, l
local sn = debug.getinfo(1).source
if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
local pt = ".*\\"
if SysUtils.PathDelim == "/" then pt = "/.*/" end
local i, j = string.find(sn, pt)
if i ~= nil then
  dbn = string.sub(sn, i, j) .. 'marker.txt'
  dbne = SysUtils.FileExists(dbn)
else
  Dialogs.MessageBox('Error 1', 'marker.lua', 0x0030)
  return
end

local db = {}
local lu = {}
local r = {}

if dbne == true then
  h, err = io.open(dbn, 'r')
  if h == nil then
    Dialogs.MessageBox('Error: ' .. err, 'marker.lua', 0x0030)
    return
  end
  local n, k, r
  for l in h:lines() do
    n = string.find(l, '|', 1, true)
    if n ~= nil then
      k = string.sub(l, 1, n - 1)
      db[k] = string.sub(l, n + 1)
    end
  end
  h:close()
end

h, err = io.open(params[2], 'r')
if h == nil then
  Dialogs.MessageBox('Error: ' .. err, 'marker.lua', 0x0030)
  return
end
for l in h:lines() do
  if SysUtils.DirectoryExists(l) == false then table.insert(lu, l) end
end
h:close()

if #lu > 0 then
  if params[1] == '--add' then
    for i = 1, #lu do db[lu[i]] = params[3] end
    for k, v in pairs(db) do table.insert(r, k .. '|' .. v) end
  elseif params[1] == '--del' then
    for i = 1, #lu do
      if db[lu[i]] ~= nil then db[lu[i]] = false end
    end
    for k, v in pairs(db) do
      if v ~= false then table.insert(r, k .. '|' .. v) end
    end
  else
    Dialogs.MessageBox('Check parameters!', 'marker.lua', 0x0030)
    return
  end
  h, err = io.open(dbn, 'w+')
  if h == nil then
    Dialogs.MessageBox('Error: ' .. err, 'marker.lua', 0x0030)
    return
  end
  if #r > 0 then h:write(table.concat(r, '\n') .. '\n') end
  h:close()
else
  Dialogs.MessageBox('Error 2', 'marker.lua', 0x0030)
  return
end
