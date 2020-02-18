-- MakeDir.lua (cross-platform)
-- 2020.02.18
--
-- Creating directories with additional features.

-- 1. You can use absolute name (/path/to/newdir), relative to current path (to/newdir) or only name (newdir).
-- 2. Script will also create all directories leading up to the entered name(s) that do not exist already.
-- 3. If you want more one dir at once then use vertical bar:
--    newdir1|newdir2|newdir3
-- 4. Also you can use some variables:
--   - <dt>: current date and time: YYYYMMDD-hhmmss;
--   - <d> or <t>: current date (YYYYMMDD) or time (hhmmss);
--   - <y>: current year, 4 digits;
--   - <p>: parent folder from current directory;
--   - <x-y>: multiple folder creation with counter. Wight depends on y.
--     For example:
--     "name<1-15>" means 15 folders: "name01", "name02", ... "name15".
--
-- Params:
--   %"0%D
--   %permission_mode%
--
-- %permission_mode% is optional parameter for Linux only, use octal mode (for example, 755 or 777)
--
-- Button example:
--[[
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{78AE07D0-42A0-4238-92BB-BE802AFB6F49}</ID>
    <Icon>cm_executescript</Icon>
    <Hint>Make dir(s)</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/utils/MakeDir.lua</Param>
    <Param>%"0%D</Param>
  </Command>
</doublecmd>
]]

local params = {...}

local function PathIsAbsolute(s, ds)
  if ds == '\\' then
    if string.match(s, '^[A-Za-z]:\\.*') ~= nil then return true end
  else
    if string.sub(s, 1, 1) == '/' then return true end
  end
  return false
end

local function MakeList(s, p, ds)
  local a = {}
  for l in string.gmatch(s, '[^|]+') do
    if PathIsAbsolute(l, ds) then
      table.insert(a, l)
    else
      table.insert(a, p .. ds .. l)
    end
  end
  return a
end

local function GetOutput(cmd)
  local h = io.popen(cmd, 'r')
  local out = h:read('*a')
  h:close()
  return string.sub(out, 1, -2)
end

local function MakeDir(a, ds)
  local m
  if ds == '/' then
    local cmd = 'mkdir -p'
    if #params == 2 then cmd = 'mkdir -m ' .. params[2] .. ' -p' end
    m = GetOutput(cmd .. ' "' .. table.concat(a, '" "') .. '" 2>&1')
  else
    m = ''
    local c = 0
    for i = 1, #a do
      if SysUtils.CreateDirectory(a[i]) == false then
        m = m .. '\n' .. a[i]
        c = c + 1
      end
    end
    if c > 0 then m = 'Error:\n' .. m end
  end
  return m
end

if (#params == 0) or (#params > 2) then
  Dialogs.MessageBox('Check the number of parameters!', 'Make dir(s)', 0x0030)
  return
end

local msg, tmp, lst
local msgl = [[
Variables:
<dt>: current date and time: YYYYMMDD-hhmmss;
<d> or <t>: current date (YYYYMMDD) or time (hhmmss);
<y>: current year, 4 digits;
<p>: parent folder from current directory;
<x-y>: multiple folder creation with counter.
]]
local ba, dn = Dialogs.InputQuery('Make dir(s)', msgl .. '\nEnter name:', false, 'new')
if ba == false then return end
local ds = SysUtils.PathDelim
local cdt = os.date('%Y%m%d-%H%M%S', os.time())
local pf = SysUtils.ExtractFileName(params[1])
-- delete trailing space(s)
dn = string.gsub(dn, '^%s+', '')
dn = string.gsub(dn, '%s+$', '')
dn = string.gsub(dn, '(%s+)([\\/|])', '%2')
dn = string.gsub(dn, '([\\/|])(%s+)', '%1')
lst = MakeList(dn, params[1], ds)
local n1, n2, c1, c2, t
local n3, c3 = 1, 1
local r = {}
local lst2 = {}
for i = 1, #lst do
  lst[i] = string.gsub(lst[i], '<dt>', cdt)
  lst[i] = string.gsub(lst[i], '<d>', string.sub(cdt, 1, 8))
  lst[i] = string.gsub(lst[i], '<t>', string.sub(cdt, 10, -1))
  lst[i] = string.gsub(lst[i], '<y>', string.sub(cdt, 1, 4))
  lst[i] = string.gsub(lst[i], '<p>', pf)
  while true do
    n1, n2 = string.find(lst[i], '<%d+%-%d+>', n3, false)
    if n1 == nil then break else table.insert(r, i) end
    c1, c2 = string.match(string.sub(lst[i], n1, n2), '<(%d+)%-(%d+)>')
    c1 = tonumber(c1)
    c2 = tonumber(c2)
    c3 = string.len(c2)
    for j = c1, c2 do
      t = j
      while true do
        if string.len(t) == c3 then
          break
        else
          t = '0' .. t
        end
      end
      table.insert(lst2, string.sub(lst[i], 1, n1 - 1) .. t .. string.sub(lst[i], n2 + 1, -1))
    end
    n3 = n2
  end
end
-- <x-y>
for i = 1, #r do table.remove(lst, r[i]) end

msg = MakeDir(lst, ds)
if #lst2 > 0 then
  tmp = MakeDir(lst2, ds)
  if (tmp ~= '') and (tmp ~= nil) then
    if (msg ~= '') and (msg ~= nil) then
      msg = msg .. string.sub(tmp, 7, -1)
    else
      msg = tmp
    end
  end
end

msg = string.gsub(msg, '\n\n+', '\n')
if string.len(msg) > 8 then
  Dialogs.MessageBox(msg, 'Make dir(s)', 0x0030)
end
