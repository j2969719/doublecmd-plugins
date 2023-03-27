-- ChangePermissionsOwnwer.lua
-- 2023.03.27
--[[
Set permissions or owner (will ask for root privileges if that doesn't work).
Script uses chmod and chown.

Params:
    %LU   - list of selected file;
  optional:
    --mod - change permissions,
    or
    --own - change owner.
]]

local params = {...}

local sn = debug.getinfo(1).source
if string.sub(sn, 1, 1) == '@' then sn = string.sub(sn, 2, -1) end
sn = SysUtils.ExtractFileName(sn)

if #params == 0 then
  Dialogs.MessageBox("Check the list of parameters!", sn, 0x0030)
  return
end

local function GetOutput(cmd)
  local h = io.popen(cmd)
  if h ~= nil then
    local out = h:read("*a")
    h:close()
    if out ~= nil then
      out = string.gsub(out, "%s+$", "")
      return out
    end
  end
  return nil
end

local aact = {
"Permissions",
"Owner"
}
local amod = {
"644 (-rw-r--r--)",
"664 (-rw-rw-r--)",
"755 (-rwxr-xr-x)",
"775 (-rwxrwxr-x)",
"777 (-rwxrwxrwx)",
"600 (-rw-------)",
"444 (-r--r--r--)",
"444 (-r--------)"
}
local aown = {
"root:root"
}

local h, err = io.open(params[1], "r")
if h == nil then
  Dialogs.MessageBox("Error: " .. err, sn, 0x0030)
  return
end
local list = {}
local i = 1
for l in h:lines() do
  list[i] = l
  i = i + 1
end
h:close()
if #list == 0 then return end

local sact, sitem, sout

if #params == 2 then
  if params[2] == "--mod" then
    sact = "Permissions"
  elseif params[2] == "--own" then
    sact = "Owner"
  end
else
  sact = Dialogs.InputListBox(sn, "Choose action:", aact, 1)
end
if sact == "Permissions" then
  sitem = Dialogs.InputListBox(sn, "Choose permissions mode:", amod, 1)
  if sitem == nil then return end
  sout = GetOutput('chmod ' .. string.sub(sitem, 1, 3) .. ' "' .. table.concat(list, '" "') .. '" 2>&1')
  if sout == nil then
    Dialogs.MessageBox("Unknown error", sn, 0x0030)
  elseif sout ~= "" then
    sout = GetOutput('pkexec chmod ' .. string.sub(sitem, 1, 3) .. ' "' .. table.concat(list, '" "') .. '" 2>&1')
    if sout == nil then
      Dialogs.MessageBox("Unknown error", sn, 0x0030)
    elseif sout ~= "" then
      Dialogs.MessageBox(sout, sn, 0x0030)
    end
  end
elseif sact == "Owner" then
  local suser = os.getenv("USER")
  aown[#aown + 1] = suser .. ":" ..suser
  aown[#aown + 1] = "Set manually"
  sitem = Dialogs.InputListBox(sn, "Choose permissions mode:", aown, 1)
  if sitem == nil then return end
  if sitem == "Set manually" then
    local bact
    bact, sitem = Dialogs.InputQuery(sn, "Owner:", false, "root:root")
    if bact == false then return end
  end
  sout = GetOutput('chown ' .. sitem .. ' "' .. table.concat(list, '" "') .. '" 2>&1')
  if sout == nil then
    Dialogs.MessageBox("Unknown error", sn, 0x0030)
  elseif sout ~= "" then
    sout = GetOutput('pkexec chown ' .. sitem .. ' "' .. table.concat(list, '" "') .. '" 2>&1')
    if sout == nil then
      Dialogs.MessageBox("Unknown error", sn, 0x0030)
    elseif sout ~= "" then
      Dialogs.MessageBox(sout, sn, 0x0030)
    end
  end
end
