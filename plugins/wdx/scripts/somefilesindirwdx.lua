-- based on someaudioext4findfiles.lua/someaudioext4columns.lua by Skif_off
-- https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=806&p=19751#p19751
-- dc >= r8553

local masks = {
    "*.aa3", "*.aac", "*.ac3", "*.adts", "*.aiff", "*.ape", "*.at3", "*.au", "*.dts", "*.dtshd", "*.flac", "*.l16", "*.m4a",
    "*.m4b", "*.m4p", "*.m4r", "*.mp", "*.mp+", "*.mp1", "*.mp2", "*.mp3", "*.mp4", "*.mpc", "*.mpp", "*.ofr", "*.oga", "*.ogg",
    "*.oma", "*.opus", "*.pcm", "*.sb0", "*.spx", "*.tak", "*.tta", "*.vqf", "*.wav", "*.wave", "*.wma", "*.wv",
}

local delim = SysUtils.PathDelim;

function ContentGetSupportedField(FieldIndex)
    if (masks[FieldIndex + 1] ~= nil) then
        return masks[FieldIndex + 1], 'default|recursively', 6;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if SysUtils.DirectoryExists(FileName) then
        if (delim == nil) then
            if (FileName:find("^/") ~= nil) then
                delim = "/";
            else
                delim = "\\";
            end
        end
        if (UnitIndex == 0) then
            return FindFiles(FileName, masks[FieldIndex + 1]);
        elseif (UnitIndex == 1) then
            return ScanDir(FileName, masks[FieldIndex + 1]);
        end
    end
    return nil;
end

function FindFiles(path, mask)
    local handle = SysUtils.FindFirst(path .. delim .. mask);
    if (handle ~= nil) then
        SysUtils.FindClose(handle);
        return true;
    end
    return false;
end

function ScanDir(path, mask)
    if (FindFiles(path, mask) == true) then
        return true;
    end
    local result = false;
    local handle, FindData = SysUtils.FindFirst(path .. delim .. '*');
    if (handle ~= nil) then
        repeat
            if (FindData.Name ~= ".") and (FindData.Name ~= "..") then
                if (math.floor(FindData.Attr / 0x00000010) % 2 ~= 0) and (ScanDir(path .. delim .. FindData.Name, mask) == true) then
                    result = true;
                    break;
                end
            end
            found, FindData = SysUtils.FindNext(handle);
        until (found == nil)
        SysUtils.FindClose(handle);
    end
    return result;
end