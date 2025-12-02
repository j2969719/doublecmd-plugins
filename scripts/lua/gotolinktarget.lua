--[[
Script (cross-platform) will check the symbolic link under the cursor and
- if the target object is a directory: will open the target directory;
- if the target object is a file: will open the parent directory and move the cursor to the target file.

Command: cm_ExecuteScript
Params:
  path-to-gotolinktarget.lua
  %"0%p0

Optional params:
  newtab    - will open the target directory in a new tab.
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

local DIALOG_TITLE = "Go to link target"

local MB_ICONERROR       = 0x0010
local MB_ICONWARNING     = 0x0030
local MB_ICONINFORMATION = 0x0040

local params = {...}
local search_delay = 1000 --[[ ms delay before putting cursor on target file (for DC older than v1.1.30) ]] / 10

if #params == 0 then
  Dialogs.MessageBox("Required parameters are missing, please check the button settings.", DIALOG_TITLE, MB_ICONERROR)
  return
end

local faSymLink   = 0x00000400
local faDirectory = 0x00000010

local function hasattr(fileattr, value)
  return (fileattr > 0 and math.floor(fileattr / value) % 2 ~= 0)
end

local filename = params[1]
local fileattr = SysUtils.FileGetAttr(filename)

if not hasattr(fileattr, faSymLink) then
  Dialogs.MessageBox('"' .. filename .. '" is not a symbolic link.', DIALOG_TITLE, MB_ICONERROR)
  return
end

local is_newtab    = false
local is_inactive  = false
local is_onlydir   = false
local is_recursive = false

if #params > 1 then
  local user_options = table.concat(params, ";", 2)
  is_newtab    = (user_options:find("newtab") ~= nil)
  is_inactive  = (user_options:find("inactive") ~= nil)
  is_onlydir   = (user_options:find("onlydir") ~= nil)
  is_recursive = (user_options:find("recursive") ~= nil)
end

local link_target = SysUtils.ReadSymbolicLink(filename, is_recursive)

if link_target == "" then
  Dialogs.MessageBox("This link is broken or the target storage is currently unavailable.", DIALOG_TITLE, MB_ICONINFORMATION)
  return
end

local is_absolute = false
local search_opts = "search=on", "direction=first", "matchbeginning=off", "files=on", "directories=off"

if SysUtils.PathDelim == "/" then
  is_absolute = (link_target:sub(1, 1) == "/")
  search_opts = search_opts, "casesensitive=on"
else
  is_absolute = (link_target:sub(2, 3) == ":\\")
  search_opts = search_opts, "casesensitive=off"
end

if not is_absolute then
  link_target = SysUtils.GetAbsolutePath(link_target, SysUtils.ExtractFileDir(filename))
end

fileattr = SysUtils.FileGetAttr(link_target)

if fileattr == -1 then
  Dialogs.MessageBox('The target object "' ..  link_target .. '" is currently unavailable.', DIALOG_TITLE, MB_ICONINFORMATION)
  return
end

local is_link_to_dir = hasattr(fileattr, faDirectory)

if is_inactive then
  DC.ExecuteCommand("cm_FocusSwap")
end

if is_newtab then
  DC.ExecuteCommand("cm_NewTab")
end

if is_link_to_dir then
  DC.ExecuteCommand("cm_ChangeDir", link_target)
elseif is_onlydir then
  DC.ExecuteCommand("cm_ChangeDir", SysUtils.ExtractFileDir(link_target))
else
  if not pcall(DC.GoToFile, link_target, true) then
    DC.ExecuteCommand("cm_ChangeDir", SysUtils.ExtractFileDir(link_target))
    for _ = 1, search_delay do
      SysUtils.Sleep(10)
      DC.ExecuteCommand("")
    end
    DC.ExecuteCommand("cm_QuickSearch", search_opts, "text=" .. SysUtils.ExtractFileName(link_target))
    DC.ExecuteCommand("cm_QuickSearch", "search=off")
  end
end
