-- based on someaudioext4findfiles.lua by Skif_off
-- https://doublecmd.sourceforge.io/forum/viewtopic.php?f=8&t=806&p=19751#p19751
-- case insensitive

local extlist = {
    "aa3", "aac", "ac3", "adts", "aiff", "ape", "at3", "au", "dts", "dtshd", "flac", "l16", "m4a",
    "m4b", "m4p", "m4r", "mp", "mp+", "mp1", "mp2", "mp3", "mp4", "mpc", "mpp", "ofr", "oga", "ogg",
    "oma", "opus", "pcm", "sb0", "spx", "tak", "tta", "vqf", "wav", "wave", "wma", "wv",
}

local delim = SysUtils.PathDelim;

function ContentGetSupportedField(FieldIndex)
    if (extlist[FieldIndex + 1] ~= nil) then
        return extlist[FieldIndex + 1], '', 6;
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
        local result = false;
        local handle, FindData = SysUtils.FindFirst(FileName .. delim .. '*');
        if (handle ~= nil) then
            repeat
                if (FindData.Name ~= ".") and (FindData.Name ~= "..") then
                    if (math.floor(FindData.Attr / 0x00000010) % 2 == 0) then
                        local ext = FindData.Name:match(".+%.(.+)$");
                        if (ext ~= nil) and (ext:lower() == extlist[FieldIndex + 1]) then
                            result = true;
                            break;
                        end
                    end
                end
                found, FindData = SysUtils.FindNext(handle);
            until (found == nil)
            SysUtils.FindClose(handle);
        end
        return result;
    end
    return nil;
end