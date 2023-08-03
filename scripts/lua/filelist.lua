--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Menu>
    <ID>{2A0B7BD5-3B71-4B7C-AD77-8B447B707BED}</ID>
    <Icon>cm_loadlist</Icon>
    <Hint>FileLists</Hint>
    <MenuItems>
      <Command>
        <ID>{DC35335E-DB81-4F8D-AF1D-31D4A43D3111}</ID>
        <Icon>cm_loadlist</Icon>
        <Hint>Load FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
      </Command>
      <Command>
        <ID>{E974E3D8-E6C5-411F-A9A1-6E8D95F4C1E8}</ID>
        <Icon>cm_loadlist</Icon>
        <Hint>Load FileList from file under cursor</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>cursor</Param>
        <Param>%"0%p0</Param>
      </Command>
      <Separator>
        <Style>False</Style>
      </Separator>
      <Command>
        <ID>{E4A17420-0A4C-461A-B642-E61F4FC327B1}</ID>
        <Icon>cm_saveselectiontofile</Icon>
        <Hint>Save Selection to FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>store</Param>
        <Param>%L</Param>
      </Command>
      <Command>
        <ID>{199E18A7-B158-490F-AF6F-0BFEC21B5C24}</ID>
        <Icon>cm_saveselectiontofile</Icon>
        <Hint>Append Selection to FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>append</Param>
        <Param>%L</Param>
      </Command>
      <Separator>
        <Style>False</Style>
      </Separator>
      <Command>
        <ID>{8DF3F858-F39B-4E62-AF9E-3515FD4AE61F}</ID>
        <Icon>cm_saveselectiontofile</Icon>
        <Hint>Save Selection from both panels to FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>store</Param>
        <Param>%Lp</Param>
      </Command>
      <Command>
        <ID>{7787B087-6C6C-45A7-A519-6206CF81D478}</ID>
        <Icon>cm_saveselectiontofile</Icon>
        <Hint>Append Selection from both panels to FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>append</Param>
        <Param>%Lp</Param>
      </Command>
      <Separator>
        <Style>False</Style>
      </Separator>
      <Command>
        <ID>{DB9A182A-9948-4B37-B702-A100084AF6EA}</ID>
        <Icon>cm_open</Icon>
        <Hint>Open FileList file</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>open</Param>
      </Command>
      <Command>
        <ID>{A19A0068-EB87-498A-8550-37195C96B704}</ID>
        <Icon>cm_sortbyname</Icon>
        <Hint>Sort lines in FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>sort</Param>
      </Command>
      <Command>
        <ID>{8734074F-7F35-4D79-A478-DEAF65349B7A}</ID>
        <Icon>cm_wipe</Icon>
        <Hint>Remove non-existent files from FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>clean</Param>
      </Command>
      <Separator>
        <Style>False</Style>
      </Separator>
      <Command>
        <ID>{7C2E2F4D-BD09-42DB-9F70-B2E1F2B0131C}</ID>
        <Icon>cm_delete</Icon>
        <Hint>Remove FileList</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>remove</Param>
      </Command>
      <Separator>
        <Style>False</Style>
      </Separator>
      <Command>
        <ID>{D9A64959-D7AA-4B50-A4E3-4766355A88D1}</ID>
        <Icon>cm_quickfilter</Icon>
        <Hint>Only "Filter" Panel</Hint>
        <Command>cm_ExecuteScript</Command>
        <Param>$DC_CONFIG_PATH/scripts/lua/filelist.lua</Param>
        <Param>filter</Param>
      </Command>
    </MenuItems>
  </Menu>
</doublecmd>

]]


local Args = {...}
local FileListsDir = os.getenv("HOME") .. "/.cache/doublecmd-filelists/"
local FileListExt = ".lst"
local DefaultFileListName = "Name"
local EditCommand = "xdg-open"
local Messages = {
    "You don't have any saved FileList.",
    "Load FileList:",
    "Save Selection to FileList:",
    "Already exists, overwrite?",
    "Nothing is selected.",
    "Remove FileList:",
    "Error reading from file",
    "Error writing to file",
    "Append Selection to FileList:",
    "Remove non-existent files from FileList:",
    "Sort lines in FileList:",
    "Open FileList:",
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

function LoadFileToTable(FileName)
    if not FileName or not SysUtils.FileExists(FileName) then
        Dialogs.MessageBox(Messages[7], '', 0x0010)
        return nil
    end
    local File = io.open(FileName, 'r')
    if (File ~= nil) then
        Result = {}
        for Line in File:lines() do
            if (Line ~= '') then
                table.insert(Result, Line)
            end
        end
        File:close()
        return Result
    else
        Dialogs.MessageBox(Messages[7], '', 0x0010)
    end
    return nil
end

function SaveTableToFile(Table, FileName)
    local File = io.open(FileName, 'w+')
    if (File ~= nil) then
        for _, Line in ipairs(Table) do
            File:write(Line .. '\n')
        end
        File:close()
    else
        Dialogs.MessageBox(Messages[8], '', 0x0010)
    end
end


if not Args[1] then
    local FileList = GetFileListFile(Messages[2])
    if (FileList ~= nil and SysUtils.FileExists(FileList)) then
        DC.ExecuteCommand("cm_LoadList", "filename=" .. FileList)
    end
elseif (Args[1] == "store") then
    local Lines = LoadFileToTable(Args[2])
    if (Lines ~= nil and #Lines > 0) then
        local _, ListName = Dialogs.InputQuery('', Messages[3], false, DefaultFileListName)
        if (ListName ~= nil and ListName ~= '') then
            local FileList = FileListsDir .. ListName .. FileListExt
            if (SysUtils.FileExists(FileList)) then
                if (Dialogs.MessageBox(Messages[4], '', QuestionFlags) == mrYes) then
                    SaveTableToFile(Lines, FileList)
                end
            else
                SaveTableToFile(Lines, FileList)
            end
        end
    else
        Dialogs.MessageBox(Messages[5], '', 0x0010)
    end
elseif (Args[1] == "remove") then
    local FileList = GetFileListFile(Messages[6])
    if (FileList ~= nil and SysUtils.FileExists(FileList)) then
        os.remove(FileList)
    end
elseif (Args[1] == "append") then
    local FileList = GetFileListFile(Messages[9])
    if (FileList ~= nil and SysUtils.FileExists(FileList)) then
        local OldLines = LoadFileToTable(FileList)
        local NewLines = LoadFileToTable(Args[2])
        if (OldLines ~= nil) and (NewLines ~= nil) then
            if (#NewLines > 0) then
                Result = {}
                for _, OldLine in ipairs(OldLines) do
                    local Found = false
                    for _, NewLine in ipairs(NewLines) do
                        if (OldLine == NewLine) then
                            Found = true
                        end
                    end
                    if not Found then
                        table.insert(Result, OldLine)
                    end
                end
                for _, NewLine in ipairs(NewLines) do
                    table.insert(Result, NewLine)
                end
                SaveTableToFile(Result, FileList)
            else
                Dialogs.MessageBox(Messages[5], '', 0x0010)
            end
        end
    end
elseif (Args[1] == "clean") then
    local FileList = Args[2]
    if not FileList then
        FileList = GetFileListFile(Messages[10])
    end
    if (FileList ~= nil and SysUtils.FileExists(FileList)) then
        local Lines = LoadFileToTable(FileList)
        if (Lines ~= nil) and (#Lines > 0) then
            for Index = #Lines, 1, -1 do
                if not SysUtils.FileExists(Lines[Index]) then
                    table.remove(Lines, Index)
                end
            end
            SaveTableToFile(Lines, FileList)
        end
    end
elseif (Args[1] == "sort") then
    local FileList = Args[2]
    if not FileList then
        FileList = GetFileListFile(Messages[11])
    end
    if (FileList ~= nil and SysUtils.FileExists(FileList)) then
        local Lines = LoadFileToTable(FileList)
        if (Lines ~= nil) and (#Lines > 0) then
            table.sort(Lines)
            SaveTableToFile(Lines, FileList)
        end
    end
elseif (Args[1] == "open") then
    local FileList = GetFileListFile(Messages[12])
    if (FileList ~= nil and SysUtils.FileExists(FileList)) then
        os.execute(EditCommand .. ' "' .. FileList:gsub('"', '\\"') .. '" &')
    end
elseif (Args[1] == "cursor") then
    DC.ExecuteCommand("cm_LoadList", "filename=" .. Args[2])
elseif (Args[1] == "filter") then
    local BackupText = Clipbrd.GetAsText()
    DC.ExecuteCommand("cm_MarkPlus")
    DC.ExecuteCommand("cm_CopyFullNamesToClip")
    local Text = Clipbrd.GetAsText()
    Clipbrd.SetAsText(BackupText)
    if Text ~= nil and Text ~= '' then
        Text = Text .. '\n'
        local TmpFile = os.tmpname()
        local File = io.output(TmpFile)
        if File ~= nil then
            for Line in Text:gmatch("[^\n]-\n") do
                File:write(Line)
            end
            File:close()
        end
        DC.ExecuteCommand("cm_LoadList", "filename=" .. TmpFile)
        os.remove(TmpFile)
    end
end
