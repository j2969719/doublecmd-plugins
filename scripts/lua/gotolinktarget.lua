--[[
Script (cross-platform) will check the symbolic link under the cursor and
- if the target object is a directory: will open the target directory;
- if the target object is a file: will open the parent directory and move the cursor to the target file.

Command: cm_ExecuteScript
Params:
  path-to-gotolinktarget.lua
  %"0%p0

Optional params:
  inactive  - will open the target directory in the inactive panel and make it active.
  onlydir   - if the target object is a file, only the parent directory will be opened.
  recursive - if the link points to a link then it's resolved recursively until a valid
              file name that is not a link is found.

Button example:

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{4651FC8B-CC8A-4D8B-8741-DB29D812B7E3}</ID>
    <Icon>cm_symlink</Icon>
    <Hint>go to link target</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$DC_CONFIG_PATH/scripts/lua/gotolinktarget.lua</Param>
    <Param>%"0%p0</Param>
    <Param>onlydir</Param>
  </Command>
</doublecmd>

]]

local params = {...}

if #params == 0 then
  Dialogs.MessageBox("Check parameters!", "Go to link target", 0x0030)
  return
end

local iattr = SysUtils.FileGetAttr(params[1])
if (iattr > 0) and (math.floor(iattr / 0x00000400) % 2 == 0) then return end

local scase, starget, stargetdir
local bfile, bfull binactive = true, true, false
local sparams = ""

if #params > 1 then sparams = table.concat(params, ";", 2) end

if string.find(sparams, "recursive", 1, true) ~= nil then
  starget = SysUtils.ReadSymbolicLink(params[1], true)
  if starget == "" then
    Dialogs.MessageBox("This link is broken or the target storage is currently unavailable.", "Go to link target", 0x0040)
    return
  end
else
  starget = SysUtils.ReadSymbolicLink(params[1], false)
end

if SysUtils.PathDelim == "/" then
  if string.sub(starget, 1, 1) ~= "/" then bfull = false end
  scase = "on"
else
  if string.sub(starget, 2, 3) ~= ":\\" then bfull = false end
  scase = "off"
end

if bfull == false then
  starget = SysUtils.GetAbsolutePath(starget, SysUtils.ExtractFileDir(params[1]))
end

iattr = SysUtils.FileGetAttr(starget)
if iattr == -1 then
  Dialogs.MessageBox('The target object\n"' ..  starget .. '"\nis currently unavailable.', "Go to link target", 0x0040)
  return
else
  if (math.floor(iattr / 0x00000010) % 2 == 0) then
    stargetdir = SysUtils.ExtractFileDir(starget)
  else
    stargetdir = starget
    bfile = false
  end
end

if string.find(sparams, "inactive", 1, true) ~= nil then binactive = true end

if string.find(sparams, "onlydir", 1, true) ~= nil then
  if binactive == true then
    DC.ExecuteCommand("cm_FocusSwap")
  end
  DC.ExecuteCommand("cm_ChangeDir", stargetdir)
  return
end

if bfile == true then
  if binactive == true then
    DC.ExecuteCommand("cm_FocusSwap")
    DC.ExecuteCommand("cm_ChangeDir", stargetdir)
  else
    if SysUtils.ExtractFileDir(params[1]) == stargetdir then
      DC.ExecuteCommand("cm_QuickFilter", "search=off")
    else
      DC.ExecuteCommand("cm_ChangeDir", stargetdir)
    end
  end
  starget = SysUtils.ExtractFileName(starget)
  DC.ExecuteCommand("cm_QuickSearch", "search=on", "direction=first", "matchbeginning=off", "matchending=off", "casesensitive=" .. scase, "files=on", "directories=off", "text=" .. starget)
  DC.ExecuteCommand("cm_QuickSearch", "search=off")
end
