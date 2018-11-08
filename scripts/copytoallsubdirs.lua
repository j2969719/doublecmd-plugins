--[[

DOUBLECMD#TOOLBAR#XMLDATA<?xml version="1.0" encoding="UTF-8"?>
<doublecmd>
  <Command>
    <ID>{767A8509-B6FD-46A4-9856-7DD0FDEC8633}</ID>
    <Icon>cm_copy</Icon>
    <Hint>copy to all subdirs (recursively)</Hint>
    <Command>cm_ExecuteScript</Command>
    <Param>/path/to/copytoallsubdirs.lua</Param>
    <Param>%Dt</Param>
  </Command>
</doublecmd>

]]

local ShowConfirmation = "no";
local QueueID = "1";
local Blacklist = {".", ".."};


local Params = {...};
local StartPath = Params[1];
local FileList  = {};


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

function FindSubdirs(Path)
    local Result = nil;
    local Handle, FindData = SysUtils.FindFirst(Path .. SysUtils.PathDelim .. "*");
    if (Handle ~= nil) then
        repeat
            if (math.floor(FindData.Attr / 0x00000010) % 2 ~= 0) and (CheckName(FindData.Name) == true) then
                local Subdir = Path .. SysUtils.PathDelim .. FindData.Name;
                table.insert(FileList, Subdir);
                FindSubdirs(Subdir);
            end
            Result, FindData = SysUtils.FindNext(Handle);
        until (Result == nil)
        SysUtils.FindClose(Handle);
    end
end

if (StartPath ~= nil) then

    FindSubdirs(StartPath);

    for i = 1, #FileList do
        if SysUtils.DirectoryExists(FileList[i]) then
            CDAndCopy(FileList[i]);
        end
    end

    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_ChangeDir", StartPath);
    DC.ExecuteCommand("cm_FocusSwap");
    DC.ExecuteCommand("cm_MarkUnmarkAll");
end
