-- marker.lua (cross-platform)
-- 2022.12.07

local params = {...}

local dbn, dbne, h, err, l
local sn = debug.getinfo(1).source
if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
dbn = SysUtils.ExtractFilePath(sn) .. 'marker.txt'
dbne = SysUtils.FileExists(dbn)

local db = {}
local lu = {}
local r = {}
local c

if dbne == true then
  h, err = io.open(dbn, 'r')
  if h == nil then
    Dialogs.MessageBox('Error: ' .. err, 'marker.lua', 0x0030)
    return
  end
  local n, k
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
c = 1
for l in h:lines() do
  if SysUtils.DirectoryExists(l) == false then
    lu[c] = l
    c = c + 1
  end
end
h:close()

if #lu > 0 then
  if params[1] == '--add' then
    for i = 1, #lu do db[lu[i]] = params[3] end
    c = 1
    for k, v in pairs(db) do
      r[c] = k .. '|' .. v
      c = c + 1
    end
  elseif params[1] == '--del' then
    for i = 1, #lu do
      if db[lu[i]] ~= nil then db[lu[i]] = false end
    end
    c = 1
    for k, v in pairs(db) do
      if v ~= false then
        r[c] = k .. '|' .. v
        c = c + 1
      end
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
  os.setenv('MarkerDB', 'Read')
else
  Dialogs.MessageBox('Error 2', 'marker.lua', 0x0030)
end
