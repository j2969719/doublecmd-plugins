-- scrtiptfileinfowdx.lua
-- https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=2727

local scriptpath = "/usr/bin/fileinfo.sh" -- default path to executable script
local skipsysfiles = true  -- skip character, block or fifo files
local delimpat = SysUtils.PathDelim;

function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
    local script = "fileinfo.sh";
    local scriptsfolder = "/scripts/";
    local pluginsfolder = "/plugins/wlx/fileinfo/";

    if (delimpat == nil) then
        delimpat = "[/\\]";
    end

    local path = IniFileName:match(".*" .. delimpat);
    if SysUtils.FileExists(path .. script) then
        scriptpath = path .. script;
    else
        path = os.getenv("COMMANDER_PATH");
        if (path ~= nil) then
            if SysUtils.FileExists(path .. scriptsfolder .. script) then
                scriptpath = path .. scriptsfolder .. script;
            elseif SysUtils.FileExists(path .. pluginsfolder .. script) then
                scriptpath = path .. pluginsfolder .. script;
            end
        end
        path = debug.getinfo(1).source;
        if (path ~= nil) then
            path = path:match("@(.*" .. delimpat .. ")");
            if SysUtils.FileExists(path .. script) then
                scriptpath = path .. script;
            end
        end
        path = os.getenv("HOME");
        if (path ~= nil) then
            path = path .. "/.config/doublecmd/";
            if SysUtils.FileExists(path .. scriptsfolder .. script) then
                scriptpath = path .. scriptsfolder .. script;
            elseif SysUtils.FileExists(path .. pluginsfolder .. script) then
                scriptpath = path .. pluginsfolder .. script;
            end
        end
    end
end

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'Text', '', 9; -- FieldName,Units,ft_fulltext
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return '(EXT="ISO")|(EXT="TORRENT")|(EXT="SO")|(EXT="MO")|(EXT="DEB")|(EXT="TAR")|(EXT="LHA")|(EXT="ARJ")|(EXT="CAB")|(EXT="HA")|(EXT="RAR")|(EXT="ALZ")|(EXT="CPIO")|(EXT="7Z")|(EXT="ACE")|(EXT="ARC")|(EXT="ZIP")|(EXT="ZOO")|(EXT="PS")|(EXT="PDF")|(EXT="ODT")|(EXT="DOC")|(EXT="XLS")|(EXT="DVI")|(EXT="DJVU")|(EXT="EPUB")|(EXT="HTML")|(EXT="HTM")|(EXT="EXE")|(EXT="DLL")|(EXT="GZ")|(EXT="BZ2")|(EXT="XZ")|(EXT="MSI")|(EXT="RTF")|(EXT="FB2")'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FileName:find(delimpat .. "%.%.$")) then
        return nil;
    end
    if (FieldIndex == 0) and (UnitIndex == 0) then
        if (skipsysfiles == true) then
            local attr = SysUtils.FileGetAttr(FileName);
            if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) then
                return nil;
            end
        end
        local handle = io.popen(scriptpath .. ' "' .. FileName:gsub('"', '\\"') .. '"', 'r');
        local result = handle:read("*a");
        handle:close();
        return result;
    end
    return nil; -- invalid
end
