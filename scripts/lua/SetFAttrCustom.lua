-- SetFAttrCustom.lua
-- 2020.08.13
--[[
Set custom extended attributes. Requires setfattr and getfattr.
For Unix-like only!

Note: If only one file is selected, then script will try to get attribute
value and use it (if exists) as default value in InputQuery.

The attribute names are stored in the "un" table (see below). These are
examples of attribute names, edit and add your names.

Params:
  %LU

Button example:

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{78AE07D0-42A0-4238-92BB-BE802AFB6F48}</ID>
    <Icon>cm_executescript</Icon>
    <Hint>Set custom extended attributes</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>%COMMANDER_PATH%/utils/SetFAttrCustom.lua</Param>
    <Param>%LU</Param>
  </Command>
</doublecmd>
]]

local params = {...}
local un = {
"comment",
"target",
"type"
}

local function GetOutput(cmd)
  local h = io.popen(cmd)
  if h ~= nil then
    local out = h:read('*a')
    h:close()
    if out ~= nil then
      if out ~= '' then out = string.gsub(out, '[\r\n]+$', '') end
      return out
    end
  end
  return nil
end

local sn = debug.getinfo(1).source
if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
sn = SysUtils.ExtractFileName(sn)

if #params ~= 1 then
  Dialogs.MessageBox('Check the number of parameters!\nRequires only one.', sn, 0x0030)
  return
end

if #un == 0 then
  Dialogs.MessageBox('Check the "un" table!', sn, 0x0030)
  return
end

local fl = {}
local h, err = io.open(params[1], 'r')
if h == nil then
  Dialogs.MessageBox('Error: ' .. err, sn, 0x0030)
  return
end
local c = 1
for l in h:lines() do
  if string.len(l) > 0 then
    fl[c] = l
    c = c + 1
  end
end
h:close()
if #fl == 0 then
  Dialogs.MessageBox('Nothing has been selected or you should check parameter.', sn, 0x0030)
  return
end

local cn = Dialogs.InputListBox(sn, 'Select name:', un, 1)
if cn == nil then return end

local t
if #fl == 1 then
  t = GetOutput('getfattr --only-values --name=user.' .. cn .. ' --encoding=text --absolute-names "' .. fl[1] .. '"')
  if t == nil then t = '' end
else
  t = ''
end

local ba, cv = Dialogs.InputQuery(sn, 'Write value:', false, t)
if ba == false then return end

local ret = 'With error:\n'
for c = 1, #fl do
  t = GetOutput('setfattr --name=user.' .. cn .. ' --value="' .. cv .. '" "' .. fl[c] .. '"')
  if (t == nil) or (t ~= '') then
    ret = ret .. fl[c] .. '\n'
  end
end
ret = string.gsub(ret, '[\r\n]+$', '')
if string.len(ret) > 11 then
  Dialogs.MessageBox(ret, sn, 0x0030)
else
  Dialogs.MessageBox('Done.', sn, 0x0040)
end
