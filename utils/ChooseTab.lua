-- ChooseTab.lua (cross-platform)
-- 2024.02.26
--[[
Show list of tabs

How to use: add button with
internal command: cm_ExecuteScript
params:
  path-to-script-ChooseTab.lua

EntitiesToUTF8():
  https://stackoverflow.com/questions/18694131/how-to-convert-utf8-byte-arrays-to-string-in-lua
]]

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == "@" then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

local aTabs = {}
local iTabCur

local function EntitiesToUTF8(dec)
  local bytemarkers = {{0x7FF, 192}, {0xFFFF, 224}, {0x1FFFFF, 240}}
  if dec < 128 then return string.char(dec) end
  local cbs = {}
  for bytes, vals in ipairs(bytemarkers) do
    if dec <= vals[1] then
      for b = bytes + 1, 2, -1 do
        local mod = dec % 64
        dec = (dec - mod) / 64
        cbs[b] = string.char(128 + mod)
      end
      cbs[1] = string.char(vals[2] + dec)
      break
    end
  end
  return table.concat(cbs)
end

local function GetTagData(s, t)
  local n1, n2 = string.find(s, "<" .. t .. ">", 1, true)
  if n1 == nil then return nil end
  n1 = string.find(s, "</" .. t .. ">", n2, true)
  s = string.sub(s, n2 + 1, n1 - 1)
  if string.len(s) > 0 then return s end
  return nil
end

local function ClearString(s)
  if string.find(s, "&", 1, true) ~= nil then
    s = string.gsub(s, "&amp;", "&")
    s = string.gsub(s, "&apos;", "'")
    s = string.gsub(s, "&#%d+;", function(e)
          return EntitiesToUTF8(tonumber(string.sub(e, 3, -2)))
        end)
    s = string.gsub(s, "&#x%x+;", function(e)
          return EntitiesToUTF8(tonumber(string.sub(e, 4, -2), 16))
        end)
  end
  local s2 = string.sub(s, -1, -1)
  if (s2 == "/") or (s2 == "\\") then
    s = string.sub(s, 1, -2)
  end
  return s
end

local function ParseTabFile(sF, sT)
  local h = io.open(sF, "rb")
  if h == nil then return end
  local sD = h:read("*a")
  h:close()
  local sDl = GetTagData(sD, sT)
  local n1, n2
  local c, n3 = 1, 1
  local tp = GetTagData(sDl, "ActiveTab")
  iTabCur = tonumber(tp) + 1
  while true do
    n1, n2 = string.find(sDl, "<Tab>", n3, true)
    if n1 == nil then break end
    n1 = string.find(sDl, "</Tab>", n2, true)
    n3 = n1
    tp = string.sub(sDl, n2, n1)
    tp = GetTagData(tp, "Path")
--    -- Check the lock status of the tab
--    tc = GetTagData(tp, "Options")
--    if tc ~= nil then
--      if (tc == "1") or (tc == "2") or (tc == "3") then
--        tp = "*" .. tp
--      end
--    end
    aTabs[c] = ClearString(tp)
    c = c + 1
  end
end

local sPanel, sPTag = "left", "Left"
local iPanel = DC.CurrentPanel()
if iPanel == 1 then
  sPanel, sPTag = "right", "Right"
end

local sTabFile = os.tmpname()
DC.ExecuteCommand("cm_SaveTabs", "filename=" .. sTabFile, "savedirhistory=0")
ParseTabFile(sTabFile, sPTag)
os.remove(sTabFile)
if #aTabs == 0 then
  Dialogs.MessageBox("Unknown error: #aTabs = 0", sScrName, 0x0030)
  return
end

local sRes, iTab = Dialogs.InputListBox(sScrName, sPTag .. " panel:", aTabs, aTabs[iTabCur])
if sRes == nil then return end
local iBut = Dialogs.MessageBox("Do you want to switch to the chosen tab?\n\nYes: switch to selected tab.\nNo: close selected tab.", sScrName, 0x0003 + 0x0020)
if (iBut ~= 0x0006) and (iBut ~= 0x0007) then
  return
end
---- Strip the lock status of the tab
--if string.sub(sRes, 1, 1) == "*" then
--  sRes = string.sub(sRes, 2, -1)
--end

-- Yes/No
DC.ExecuteCommand("cm_ActivateTabByIndex", "side=" .. sPanel, "index=" .. iTab)
-- No
if iBut == 0x0007 then
  DC.ExecuteCommand("cm_CloseTab")
  if iTabCur > iTab then
    iTabCur = iTabCur - 1
  end
  DC.ExecuteCommand("cm_ActivateTabByIndex", "side=" .. sPanel, "index=" .. iTabCur)
end
