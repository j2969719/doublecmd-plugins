--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{767A8509-B6FD-46A4-9856-7DD0FDEC8633}</ID>
    <Icon>cm_copy</Icon>
    <Hint>copy to subdirs</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>$COMMANDER_PATH/scripts/lua/copytosubdirs.lua</Param>
    <Param>%Dt</Param>
    <Param>%Lt</Param>
  </Command>
</doublecmd>

]]

local ShowConfirmation = "no";
local QueueID = "1";
local CopyToAllSubdirs = false;
local Blacklist = {".", ".."};
local Whitelist = {};
local WhitelistContainPatterns = false;


local Params = {...};
local StartPath = Params[1];
local FileList  = Params[2];
local SelectedDirs = {};
local Result = nil;
local MessageText = "Copy to " .. StartPath .. " subdirs?";
local MessageCaption = "copy to subdirs";
local Flags = 0x0004 + 0x0020;


function CDAndCopy(Directory)
    DC.ExecuteCommand("cm_FocusSwap");
    local Target = Directory:gsub(' ', '\\ ');
    DC.ExecuteCommand("cm_ChangeDir", Target);
    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_SaveSelection");
    DC.ExecuteCommand("cm_Copy", "confirmation=" .. ShowConfirmation, "queueid=" .. QueueID);
    DC.ExecuteCommand("cm_RestoreSelection");
end

function CheckName(Name)
    if (#Whitelist ~= 0) then
        for i = 1, #Whitelist do
            if (Name == Whitelist[i]) or (WhitelistContainPatterns == true and string.find(Name, Whitelist[i])) then
                return true;
            end
        end
        return false;
    end
    for i = 1, #Blacklist do
        if (Name == Blacklist[i]) then
            return false;
        end
    end
    return true;
end

local MBReturn = Dialogs.MessageBox(MessageText, MessageCaption, Flags);

if (StartPath ~= nil) and (MBReturn == 0x0006) then
    
    if (FileList ~= nil) and (CopyToAllSubdirs == false) then
        for Target in io.lines(FileList) do
            local TargetName = string.match(Target, "[" .. SysUtils.PathDelim .. "]([^" .. SysUtils.PathDelim .. "]+)$");
            if SysUtils.DirectoryExists(Target) and (CheckName(TargetName) == true) then
                table.insert(SelectedDirs, Target);
            end
        end
    end
    
    if (#SelectedDirs <= 1) or (CopyToAllSubdirs == true) then
        local Handle, FindData = SysUtils.FindFirst(StartPath .. SysUtils.PathDelim .. "*");
        if (Handle ~= nil) then
            repeat
                if (math.floor(FindData.Attr / 0x00000010) % 2 ~= 0) and (CheckName(FindData.Name) == true) then
                    CDAndCopy(StartPath .. SysUtils.PathDelim .. FindData.Name);
                end
                Result, FindData = SysUtils.FindNext(Handle);
            until (Result == nil)
            SysUtils.FindClose(Handle);
        end
    else
        for i = 1, #SelectedDirs do
            CDAndCopy(SelectedDirs[i]);
        end
    end
    
    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_ChangeDir", StartPath);
    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_MarkUnmarkAll");
end
