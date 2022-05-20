-- LoadSelectionFromClipEx.lua
-- 2022.03.10
--[[
Extend cm_LoadSelectionFromClip:
replace name(s) from clipboard on list of masks ("name.ext" > "name.*") and call cm_MarkPlus.
]]

local function GetMask(s)
  local sn = SysUtils.ExtractFileExt(s)
  local n1 = string.len(sn)
  return string.sub(s, 1, n1 * -1) .. '*'
end

local tmp = debug.getinfo(1).source
if string.sub(tmp, 1, 1) == '@' then tmp = string.sub(tmp, 2, -1) end
local sname = SysUtils.ExtractFileName(tmp)

local sbuf = Clipbrd.GetAsText()
if type(sbuf) ~= 'string' then
  Dialogs.MessageBox('Unknown error.\nTry to check the contents of the clipboard.', sname, 0x0030)
  return
end

local sdelim = ''
local smask = ''
if string.find(sbuf, '\r\n', 1, true) ~= nil then
  sdelim = '\r\n'
elseif string.find(sbuf, '\n', 1, true) ~= nil then
  sdelim = '\n'
elseif string.find(sbuf, '\r', 1, true) ~= nil then
  sdelim = '\r'
end

if sdelim == '' then
  smask = GetMask(sbuf)
else
  local n1, n2, n3, c = 1, 1, 1, 1
  local amask = {}
  while true do
    n1, n2 = string.find(sbuf, sdelim, n3, true)
    if n1 ~= nil then
      tmp = string.sub(sbuf, n3, n1 - 1)
      amask[c] = GetMask(tmp)
      c = c + 1
    else
      tmp = string.sub(sbuf, n3, -1)
      amask[c] = GetMask(tmp)
      break
    end
    n3 = n2 + 1
  end
  if #amask > 0 then smask = table.concat(amask, ';') else return end
end
DC.ExecuteCommand('cm_MarkPlus', 'mask=' .. smask , 'casesensitive=false')
