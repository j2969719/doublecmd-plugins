-- MakeDir.lua (cross-platform)
-- 2023.08.30
--
-- Creating directory(ies) with additional features.
--[[
1. You can use absolute name (/path/to/newdir), relative to current path (to/newdir) or only name (newdir).
  Script will also create all directories leading up to the entered name(s) that do not exist already.
2. If you want more one dir at once then use vertical bar:
  newdir1|newdir2|newdir3
3. All space characters (usual space characters) at the beginning or end of the name(s) will be deleted.
4. You can use some variables:
    <dt> - current date and time: YYYYMMDDhhmmss;
    <d> or <t> - current date (YYYYMMDD) or time (hhmmss);
    <y> - current year, 4 digits;
    <fdt>, <fd>, <ft> and <fy> - like previouses, but from file under cursor (last modified date/time);
    <p> - parent folder from current directory;
    <x-y> - multiple folder creation with counter. Wight depends on y.
    For example: "name<1-15>" means 15 folders: "name01", "name02", ... "name15".
  Note: You can use more then one variable in each name.
5. Use "<c>" to get the names from the clipboard.
6. If the first symbol is ":" (without quotes), this script will open the first directory.

Params:
  %"0%D
  %"0%p0
  %permissions%

where:
  %"0%D is current directory;
  and optional:
    %"0%p0
      for getting name and last modified date/time from file under cursor
      (if not exists or empty then will use name "New" and current date/time);
    %permissions%
      for Linux only, use octal mode (for example, 755 or 777).
]]

local params = {...}

local function PathIsAbsolute(s)
  if SysUtils.PathDelim == "\\" then
    if string.match(s, "^[A-Za-z]:\\.*") ~= nil then return true end
  else
    if string.sub(s, 1, 1) == "/" then return true end
  end
  return false
end

local function GetOutput(cmd)
  local h = io.popen(cmd, "r")
  local out = h:read("*a")
  h:close()
  return string.sub(out, 1, -2)
end

local function MakeDir(a, m)
  local e = {}
  local r
  if m ~= nil then
    r = GetOutput('mkdir -m ' .. m .. ' -p "' .. table.concat(a, '" "') .. '" 2>&1')
  else
    for i = 1, #a do
      if SysUtils.CreateDirectory(a[i]) == false then
        table.insert(e, a[i])
      end
    end
    if #e > 0 then r = "Error:\n" .. table.concat(e, "\n") end
  end
  return r
end

local nm = "New"
local fdt, pf, fn, pm
local cdt = os.date("%Y%m%d%H%M%S", os.time())

if #params == 3 then
  if string.find(params[2], "^%d%d+") == nil then
    if PathIsAbsolute(params[2]) then fn = params[2] end
    pm = params[3]
  else
    if PathIsAbsolute(params[3]) then fn = params[3] end
    pm = params[2]
  end
elseif #params == 2 then
  if string.find(params[2], "^%d%d+") == nil then
    if PathIsAbsolute(params[2]) then fn = params[2] end
  else
    pm = params[2]
  end
else
  if #params ~= 1 then
    Dialogs.MessageBox("Check parameters!", "Make dir(s)", 0x0030)
    return
  end
end

if fn == nil then
  fdt = cdt
else
  local h, d = SysUtils.FindFirst(fn)
  if h == nil then
    fdt = cdt
  else
    SysUtils.FindClose(h)
    if (math.floor(d.Attr / 0x00000010) % 2 ~= 0) then
      nm = d.Name
    else
      nm = string.sub(d.Name, 1, string.len(d.Name) - string.len(SysUtils.ExtractFileExt(d.Name)))
      if nm == "" then nm = d.Name end
    end
    fdt = os.date("%Y%m%d%H%M%S", d.Time)
  end
end
pf = SysUtils.ExtractFileName(params[1])

local b, ret
local msg = [[
Variables:
<dt>: current date and time: YYYYMMDDhhmmss;
<d> or <t>: current date (YYYYMMDD) or time (hhmmss);
<y>: current year, 4 digits;
<fdt>, <fd>, <ft>, <fy>: last modified date/time from file under cursor;
<p>: parent folder from current directory;
<x-y>: multiple folder creation with counter.
---
<c>: name(s) from clipboard.
-------
Add ":" to the beginning and the script will open the first directory.
]]
local ba, dn = Dialogs.InputQuery("Make dir(s)", msg .. "\nEnter name:", false, nm)
if ba == false then return end
if dn == "<c>" then
  nm = Clipbrd.GetAsText()
  if nm == "" then
    Dialogs.MessageBox("The clipboard does not contain text.", "Make dir(s)", 0x0030)
    return
  else
    nm = string.gsub(nm, "%s+$", "")
    nm = string.gsub(nm, "^%s+", "")
    nm = string.gsub(nm, "[\t ]*[\r\n]+[\t ]*", "|")
    ba, dn = Dialogs.InputQuery("Make dir(s)", msg .. "\nEnter name:", false, nm)
    if ba == false then return end
  end
end
if string.sub(dn, 1, 1) == ":" then
  dn = string.sub(dn, 2, -1)
  b = true
end
-- delete trailing space(s)
dn = string.gsub(dn, "%s+$", "")
dn = string.gsub(dn, "^%s+", "")
dn = string.gsub(dn, "(%s+)([\\/|])", "%2")
dn = string.gsub(dn, "([\\/|])(%s+)", "%1")

local lst = {}
local res = {}
local r = {}
local n1, n2, c1, c2, t
local n3, c3 = 1, 1
for l in string.gmatch(dn, "[^|]+") do
  if PathIsAbsolute(l) then
    table.insert(lst, l)
  else
    table.insert(lst, params[1] .. SysUtils.PathDelim .. l)
  end
end
for i = 1, #lst do
  lst[i] = string.gsub(lst[i], "<dt>", cdt)
  lst[i] = string.gsub(lst[i], "<d>", string.sub(cdt, 1, 8))
  lst[i] = string.gsub(lst[i], "<t>", string.sub(cdt, 9, 14))
  lst[i] = string.gsub(lst[i], "<y>", string.sub(cdt, 1, 4))
  lst[i] = string.gsub(lst[i], "<fdt>", fdt)
  lst[i] = string.gsub(lst[i], "<fd>", string.sub(fdt, 1, 8))
  lst[i] = string.gsub(lst[i], "<ft>", string.sub(fdt, 9, 14))
  lst[i] = string.gsub(lst[i], "<fy>", string.sub(fdt, 1, 4))
  lst[i] = string.gsub(lst[i], "<p>", pf)
  -- <x-y>
  while true do
    n1, n2 = string.find(lst[i], "<%d+%-%d+>", n3, false)
    if n1 == nil then
      table.insert(res, lst[i])
      break
    end
    c1, c2 = string.match(string.sub(lst[i], n1, n2), "<(%d+)%-(%d+)>")
    c1 = tonumber(c1)
    c2 = tonumber(c2)
    c3 = string.len(c2)
    for j = c1, c2 do
      t = j
      while true do
        if string.len(t) == c3 then break end
        t = "0" .. t
      end
      table.insert(res, string.sub(lst[i], 1, n1 - 1) .. t .. string.sub(lst[i], n2 + 1, -1))
    end
    n3 = n2
  end
  n3 = 1
end

if #res > 0 then
  ret = MakeDir(res, pm)
  if (ret ~= "") and (ret ~= nil) then
    ret = string.gsub(ret, "\n\n+", "\n")
    Dialogs.MessageBox(ret, "Make dir(s)", 0x0030)
  else
    if b == true then
      DC.ExecuteCommand("cm_ChangeDir", "activepath=" .. res[1])
    end
  end
else
  Dialogs.MessageBox("Unknown error.", "Make dir(s)", 0x0030)
end
