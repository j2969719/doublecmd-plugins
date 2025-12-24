--[[

https://github.com/doublecmd/doublecmd/discussions/2544
  
DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{249D1A6C-B499-4122-BB80-79B7AE77EA35}</ID>
    <Icon>edit-cut</Icon>
    <Hint>ffmpeg clip</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$DC_CONFIG_PATH/scripts/lua/ffmegclip.lua</Param>
    <Param>ffmpeg -i "$OLDFILE" -vcodec copy -acodec copy -ss %[Start;00:00:00] -to %[End;00:00:42] "$NEWFILE"</Param>
    <Param>%"0%p0</Param>
    <Param>_clip</Param>
  </Command>
</doublecmd>
  
]]

local params = {...}
local command = params[1]
local oldfile = params[2]
local suffix  = params[3]
local prefix  = params[4]
local max_count = 100
if not oldfile or not command or command == '' then
  return
end
if not suffix then
  suffix = "_new"
end
if not prefix then
  prefix = ""
end
local path = SysUtils.ExtractFilePath(oldfile)
local ext = SysUtils.ExtractFileExt(oldfile)
local newname_without_ext = SysUtils.ExtractFileName(oldfile)
local newname_without_ext = newname_without_ext:sub(1, -(ext:len() + 1))
local newname_without_ext = prefix .. newname_without_ext .. suffix
local newfile = ''
local tempname = path .. newname_without_ext .. ext
if not SysUtils.FileExists(tempname) then
  newfile = tempname
else
  for n = 1, max_count do
    tempname = path .. newname_without_ext .. " (" .. n .. ")" .. ext
    if not SysUtils.FileExists(tempname) then
      newfile = tempname
      break
    end
  end
end
if newfile ~= '' then
  os.setenv("OLDFILE", oldfile)
  os.setenv("NEWFILE", newfile)
  os.execute(command)
end