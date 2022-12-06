-- tags.lua (cross-platform)
-- 2022.12.05

local params = {...}

if #params == 0 then
  Dialogs.MessageBox('Check parameters!', 'tags.lua', 0x0030)
  return
end

local function ClearTag(s)
  s = string.gsub(s, '^%s+', '')
  s = string.gsub(s, '%s+$', '')
  return s
end

local function GetList(s, d1, d2, b1)
  local ar = {}
  local n1, n2 = 1, 1
  local t
  while true do
    n1 = string.find(s, d1, n2, true)
    if n1 ~= nil then
      t = string.sub(s, n2, n1 - 1)
      if t ~= '' then table.insert(ar, ClearTag(t)) end
    else
      t = string.sub(s, n2, -1)
      if t ~= '' then table.insert(ar, ClearTag(t)) end
      break
    end
    n2 = n1 + 1
  end
  local arr = {}
  if #ar > 1 then
    table.sort(ar)
    for i = 1, #ar do
      if ar[i] ~= ar[i + 1] then table.insert(arr, ar[i]) end
    end
  else
    table.insert(arr, ar[1])
  end
  if b1 == true then
    return arr
  else
    if #arr == 1 then return arr[1] else return table.concat(arr, d2) end
  end
end

local dbn, h, err, l
local sn = debug.getinfo(1).source
if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
dbn = SysUtils.ExtractFilePath(sn) .. 'tags.txt'

local db = {}
local lu = {}
local r = {}

if SysUtils.FileExists(dbn) == true then
  h, err = io.open(dbn, 'r')
  if h == nil then
    Dialogs.MessageBox('Error 1: ' .. err, 'tags.lua', 0x0030)
    return
  end
  local n, k, r
  for l in h:lines() do
    n = string.find(l, '|', 1, true)
    if n ~= nil then
      k = string.sub(l, 1, n - 1)
      db[k] = string.sub(l, n, -1)
    end
  end
  h:close()
end

local ba, sa, t

if params[1] == '--filter' then
  local db2 = {}
  local fd
  t = '|'
  for k, v in pairs(db) do
    fd = SysUtils.ExtractFileDir(k)
    if fd == params[2] then
      db2[k] = v
      t = t .. string.sub(v, 2, -1)
    end
  end
  if t == '|' then
    Dialogs.MessageBox('Files not found.', 'tags.lua', 0x0040)
    return
  end
  local af = GetList(string.sub(t, 2, -2), '|', '|', true)
  sa = Dialogs.InputListBox('tags.lua', 'Select tag:', af, af[1])
  if sa == nil then return end
  local fn = ''
  for k, v in pairs(db2) do
    if string.find(v, '|' .. sa .. '|', 1, true) ~= nil then
      fn = fn .. ';' .. SysUtils.ExtractFileName(k)
    end
  end
  if fn == '' then
    Dialogs.MessageBox('Files not found.', 'tags.lua', 0x0040)
  else
    DC.ExecuteCommand('cm_QuickFilter', 'filter=on', 'matchbeginning=off', 'matchending=off', 'files=on', 'directories=on', 'text=' .. string.sub(fn, 2, -1))
  end
  return

elseif params[1] == '--change' then
  t = db[params[2]]
  if t == nil then return end
  t = GetList(string.sub(t, 2, -2), '|', ', ', false)
  ba, sa = Dialogs.InputQuery('tags.lua', 'Change tag (separated by ","):', false, t)
  if ba == false then return end
  db[params[2]] = '|' .. GetList(sa, ',', '|', false) .. '|'
else

  h, err = io.open(params[2], 'r')
  if h == nil then
    Dialogs.MessageBox('Error 2: ' .. err, 'tags.lua', 0x0030)
    return
  end
  for l in h:lines() do table.insert(lu, l) end
  h:close()
  if #lu == 0 then
    Dialogs.MessageBox('Error 3', 'tags.lua', 0x0030)
    return
  end

  if params[1] == '--add' then
    ba, sa = Dialogs.InputQuery('tags.lua', 'Add tag (separated by ","):', false, '')
    if ba == false then return end
    sa = GetList(sa, ',', '|', false)
    for i = 1, #lu do
      t = db[lu[i]]
      if t ~= nil then
        t = t .. sa
        t = GetList(string.sub(t, 2, -1), '|', '|', false)
        db[lu[i]] = '|' .. t .. '|'
      else
        db[lu[i]] = '|' .. sa .. '|'
      end
    end

  elseif params[1] == '--del' then
    local tl = '|'
    for i = 1, #lu do
      t = db[lu[i]]
      if t ~= nil then tl = tl .. string.sub(t, 2, -1) end
    end
    if tl == '|' then
      Dialogs.MessageBox('Files not found.', 'tags.lua', 0x0040)
      return
    end
    local af2 = GetList(string.sub(tl, 2, -2), '|', '|', true)
    sa = Dialogs.InputListBox('tags.lua', 'Select tag:', af2, af2[1])
    if sa == nil then return end
    local nd3, nd4
    for i = 1, #lu do
      t = db[lu[i]]
      if t ~= nil then
        if t == '|' .. sa .. '|' then
          db[lu[i]] = false
        else
          nd3, nd4 = string.find(t, '|' .. sa .. '|', 1, true)
          if nd3 ~= nil then
            t = string.sub(t, 1, nd3) .. string.sub(t, nd4 + 1, -1)
            if t == '' then
              db[lu[i]] = false
            else
              db[lu[i]] = t
            end
          end
        end
      end
    end

  elseif params[1] == '--del-all' then
    for i = 1, #lu do
      if db[lu[i]] ~= nil then db[lu[i]] = false end
    end
  end
end

for k, v in pairs(db) do
  if v ~= false then table.insert(r, k .. v) end
end

h, err = io.open(dbn, 'w+')
if h == nil then
  Dialogs.MessageBox('Error 4: ' .. err, 'tags.lua', 0x0030)
  return
end
if #r == 0 then
  h:write('\n')
else
  h:write(table.concat(r, '\n') .. '\n')
end
h:close()

if params[3] == '--auto' then
  DC.ExecuteCommand('cm_Refresh')
end
