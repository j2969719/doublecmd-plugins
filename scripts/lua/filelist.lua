--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Menu>
    <ID>{9FE5D336-29D4-4345-81B4-01FD0D440006}</ID>
    <Icon>cm_loadlist</Icon>
    <Hint>FileLists</Hint>
    <MenuItems>
      <Command>
        <ID>{8C3648EF-B959-480A-A201-07E96CCC41DF}</ID>
        <Icon>cm_changedir</Icon>
        <Hint>Load FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
      </Command>
      <Command>
        <ID>{60E4FAC3-E9FF-4627-B2FE-1E6F80EFC745}</ID>
        <Icon>cm_saveselectiontofile</Icon>
        <Hint>Save Selection to FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>store</Param>
      </Command>
      <Command>
        <ID>{49D3038F-F0F3-4273-B094-B2D86753766B}</ID>
        <Icon>cm_delete</Icon>
        <Hint>Remove FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>remove</Param>
      </Command>
    </MenuItems>
  </Menu>
</doublecmd>

]]


local Args = {...};
local FileListsDir = os.getenv("HOME") .. "/.cache/doublecmd-filelists/"
local FileListExt = ".lst"
local DefaultFileListName = "Name"
local Messages = {
    "You don't have any saved FileList.",
    "Load FileList:",
    "Save Selection to FileList:",
    "Already exists, overwrite?",
    "Nothing is selected.",
    "Remove FileList:",
}

local MB_ICONWARNING = 0x0030
local MB_ICONERROR = 0x0010
local QuestionFlags = 0x0024
local mrYes = 0x0006

if not SysUtils.DirectoryExists(FileListsDir) then
    SysUtils.CreateDirectory(FileListsDir)
end

function SelectFileList(Msg)
    local Files = {}
    local Handle, FindData = SysUtils.FindFirst(FileListsDir .. '*' .. FileListExt);
    if (Handle ~= nil) then
        repeat
            if (math.floor(FindData.Attr / 0x00000010) % 2 == 0) then
                Name = FindData.Name:gsub(FileListExt, "")
                table.insert(Files, Name)
            end
            Result, FindData = SysUtils.FindNext(Handle);
        until (Result == nil)
        SysUtils.FindClose(Handle);
    end
    if (#Files > 0) then
        return Dialogs.InputListBox('', Msg, Files, '')
    else
        Dialogs.MessageBox(Messages[1], '', MB_ICONWARNING)
    end
    return nil
end

function GetFileListFile(Msg)
    ListName = SelectFileList(Msg)
    if (ListName ~= nil) then
        return FileListsDir .. ListName .. FileListExt
    end
    return nil
end

function SaveTextToFile(Text, FileName)
    local File = io.open(FileName, 'w+')
    if (File ~= nil) then
        File:write(Text)
        File:close()
    end
end

if not Args[1] then
    FileList = GetFileListFile(Messages[2])
    if (FileList ~= nil and SysUtils.FileExists(FileList)) then
        DC.ExecuteCommand("cm_LoadList", "filename=" .. FileList)
    end
elseif (Args[1] == "store") then
    DC.ExecuteCommand("cm_CopyFullNamesToClip")
    Text = Clipbrd.GetAsText()
    if (Text ~= nil and Text ~= nil) then
        _, ListName = Dialogs.InputQuery('', Messages[3], false, DefaultFileListName)
        if (ListName ~= nil and ListName ~= nil) then
            FileList = FileListsDir .. ListName .. FileListExt
            if (SysUtils.FileExists(FileList)) then
                if (Dialogs.MessageBox(Messages[4], '', QuestionFlags) == mrYes) then
                    SaveTextToFile(Text, FileList)
		end
            else
                SaveTextToFile(Text, FileList)
            end
        end
    else
        Dialogs.MessageBox(Messages[5], '', 0x0010)
    end
elseif (Args[1] == "remove") then
    FileList = GetFileListFile(Messages[6])
    if (FileList ~= nil and SysUtils.FileExists(FileList)) then
        os.remove(FileList)
    end
end