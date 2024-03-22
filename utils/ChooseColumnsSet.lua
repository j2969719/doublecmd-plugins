-- ChooseColumnsSet.lua (cross-platform)
-- 2024.03.10
--[[
Show list of columns sets.

How to use: add button with
internal command: cm_ExecuteScript
params:
  path-to-script-ChooseColumnsSet.lua

EntitiesToUTF8():
  https://stackoverflow.com/questions/18694131/how-to-convert-utf8-byte-arrays-to-string-in-lua
]]

local sScrName = debug.getinfo(1).source
if string.sub(sScrName, 1, 1) == "@" then
  sScrName = string.sub(sScrName, 2, -1)
end
sScrName = SysUtils.ExtractFileName(sScrName)

local sData

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

local function ClearString(s)
  if string.find(s, "&", 1, true) ~= nil then
    s = string.gsub(s, "&amp;", "&")
    s = string.gsub(s, "&apos;", "'")
    s = string.gsub(s, "&lt;", "<")
    s = string.gsub(s, "&gt;", ">")
    s = string.gsub(s, "&#%d+;", function(e)
          return EntitiesToUTF8(tonumber(string.sub(e, 3, -2)))
        end)
    s = string.gsub(s, "&#x%x+;", function(e)
          return EntitiesToUTF8(tonumber(string.sub(e, 4, -2), 16))
        end)
  end
  return s
end

local function GetName(n)
  local n1, n2 = string.find(sData, "<Name>", n, true)
  n1 = string.find(sData, "</Name>", n2, true)
  local s = string.sub(sData, n2 + 1, n1 - 1)
  n2 = string.find(sData, "<FileSystem>", n1, true)
  if string.sub(sData, n2 + 12, n2 + 26) == "&lt;General&gt;" then
    return s
  else
    return nil
  end
end

local sConfigFile = os.getenv("COMMANDER_INI")
if sConfigFile == nil then
  Dialogs.MessageBox("Unknown error: sConfigFile == nil", sScrName, 0x0030)
  return
end
local h, err = io.open(sConfigFile, "rb")
if h == nil then
  Dialogs.MessageBox("Error: " .. err, sScrName, 0x0030)
  return
end
sData = h:read("*a")
h:close()

local aSets = {}

local n1, n2, sN
local c = 1
local n3 = string.find(sData, "<ColumnsSets ", 1, true)
while true do
  n1, n2 = string.find(sData, "<ColumnsSet ", n3, true)
  if n1 == nil then break end
  sN = GetName(n2)
  if sN ~= nil then
    aSets[c] = ClearString(sN)
    c = c + 1
  end
  n3 = n2
end
if #aSets == 0 then
  Dialogs.MessageBox("Unknown error: #aSets = 0", sScrName, 0x0030)
  return
end

local sRes = Dialogs.InputListBox(sScrName, "Columns sets:", aSets, aSets[1])
if sRes ~= nil then
  DC.ExecuteCommand("cm_ColumnsView", "columnset=" .. sRes)
end
