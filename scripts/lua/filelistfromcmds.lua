--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{45516A11-A103-4BEB-B228-5BF1A9D10006}</ID>
    <Icon>cm_flatview</Icon>
    <Hint>External panelize</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$DC_CONFIG_PATH/scripts/lua/filelistfromcmds.lua</Param>
    <Param>git modified: cd %D &amp;&amp; git ls-files -m</Param>
    <Param>git conflicts: cd %D &amp;&amp; git diff --name-only --diff-filter=U --relative</Param>
    <Param>git untracked: cd %D &amp;&amp; git ls-files -o</Param>
    <Param>rejected patches: find %D \( -name \*.rej -o -name \*.orig \) -print</Param>
  </Command>
</doublecmd>

]]

local MB_ICONERROR       = 0x0010
local MB_ICONINFORMATION = 0x0040

local langs = {}

langs.en = {
  DIALOG_TITLE    = "External panelize",
  MSG_ARG_MISSING = 'Required parameters are missing, please check the button settings.',
  MSG_CMD_EMPTY   = 'Unable to find command to execute.',
  MSG_CMD_SELECT  = 'Select command',
  MSG_DC_TOO_OLD  = 'This script requires a newer version of Double Commander.',
}

langs.ru = {
  DIALOG_TITLE    = "Внешняя панельизация",
  MSG_ARG_MISSING = 'Отсутствуют обязательные параметры, проверьте настройки кнопки.',
  MSG_CMD_EMPTY   = 'Не удалось найти команду для выполнения.',
  MSG_CMD_SELECT  = 'Выберите команду',
  MSG_DC_TOO_OLD  = 'Этому скрипту нужна более новая версия Double Commander.',
}

local lang = os.getenv("LANG")
if lang then
  lang = lang:match("^%l+")
end

local function gettext(text)
  if not lang or not langs[lang] or not langs[lang][text] then
    return langs["en"][text]
  end
  return langs[lang][text]
end

local function is_relative(filename)
  if SysUtils.PathDelim == "/" then
    return (filename:sub(1, 1) ~= "/")
  end
return (filename:sub(2, 3) ~= ":\\")
end

local function open_file(filename)
  file = io.open(filename, 'r')
  if file then
    local filesize = file:seek("end")
    if (filesize > 0) then
      file:seek("set")
      return file
    else
      file:close()
    end
  end
  return nil
end

local function write_to_file(text, filename)
  local file = io.open(filename, 'w')
  if file then
    file:write(text)
    file:close()
  end
end

if not DC.ExpandVar then
  Dialogs.MessageBox(gettext("MSG_DC_TOO_OLD"), gettext("DIALOG_TITLE"), MB_ICONINFORMATION)
  return  
end

local args = {...}
local commands = {}
local labels = {}

for i = 1, #args do
  if args[i] ~= '' then   
    local label = args[i]:match("([^:]+):%s")
    if not label then
      table.insert(commands, args[i])
      table.insert(labels, args[i])
    else
      table.insert(commands, args[i]:match(":%s+(.+)"))
      table.insert(labels, label)
    end
  end
end

local command = nil
if #commands < 1 then
  Dialogs.MessageBox(gettext("MSG_ARG_MISSING"), gettext("DIALOG_TITLE"), MB_ICONINFORMATION)
  return
elseif #commands == 1 then
  command = commands[1]
else
  local last_item = ''
  local filename = os.getenv("COMMANDER_INI_PATH") .. SysUtils.PathDelim .. debug.getinfo(1).source:match("[^" .. SysUtils.PathDelim .. "]+$") .. '.txt'
  local file = open_file(filename)
  if file then
    last_item = file:read("*line")
    file:close()
  end
  local got, index = Dialogs.InputListBox(gettext("DIALOG_TITLE"), gettext("MSG_CMD_SELECT"), labels, last_item)
  if not got then
    return
  else
    write_to_file(got, filename)
    command = commands[index]
  end
end

if command == '' then
  Dialogs.MessageBox(gettext("MSG_CMD_EMPTY"), gettext("DIALOG_TITLE"), MB_ICONERROR)
  return
end

filename = SysUtils.GetTempName()
command = DC.ExpandVar(command) .. ' > "' .. filename .. '"'
os.execute(command)

local file = open_file(filename)
if file then
  local newlist = ''
  local workdir = DC.ExpandVar('%D')
  for line in file:lines() do
    local path = line
    if is_relative(path) then
      path = SysUtils.GetAbsolutePath(line, workdir)
    end
    if SysUtils.FileGetAttr(path) ~= -1 then
      newlist = newlist .. '\n' .. path
    end
  end
  file:close()
  if newlist ~= '' then
    write_to_file(newlist, filename)
    DC.ExecuteCommand("cm_LoadList", "filename=" .. filename)
  end
end 