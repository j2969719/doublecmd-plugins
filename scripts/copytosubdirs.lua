--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{767A8509-B6FD-46A4-9856-7DD0FDEC8633}</ID>
    <Icon>cm_copy</Icon>
    <Hint>copy to subdirs</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>/path/to/copytosubdirs.lua</Param>
    <Param>%Dt</Param>
    <Param>%Lt</Param>
  </Command>
</doublecmd>

]]

local ShowConfirmation = "no";
local QueueID = "1";
local CopyToAllSubdirs = false;
local Blacklist = {".", ".."};


local Params = {...};
local StartPath = Params[1];
local FileList  = Params[2];
local Counter = 0;
local Result = nil;


function CDAndCopy(Directory)
    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_ChangeDir", Directory);
    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_SaveSelection");
    DC.ExecuteCommand("cm_Copy", "confirmation=" .. ShowConfirmation, "queueid=" .. QueueID);
    DC.ExecuteCommand("cm_RestoreSelection");
end

function CheckName(Name)
    for i = 1, #Blacklist do
        if (Name == Blacklist[i]) then
            return false;
        end
    end
    return true;
end

if (StartPath ~= nil) then
    
    if (FileList ~= nil) and (CopyToAllSubdirs == false) then
        for Target in io.lines(FileList) do 
            if SysUtils.DirectoryExists(Target) then
                CDAndCopy(Target);
                Counter = Counter + 1;
            end
        end
    end
      
    if (Counter == 0) or (CopyToAllSubdirs == true) then
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
    end
    
    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_ChangeDir", StartPath);
    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_MarkUnmarkAll");
end
