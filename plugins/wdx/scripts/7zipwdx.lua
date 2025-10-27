-- 7zipwdx.lua

local z7cmd = "7z l"
local output = ''
local cached_file = ''

local fields = {
    {"Type",         8, "\nType%s=%s([^\n]+)",                    false},
    {"Method",       8, "\nMethod%s=%s([^\n]+)",                  false},
    {"Solid",        8, "\nSolid%s=%s([^\n]+)",                   false},
    {"Offset",       8, "\nOffset%s=%s([^\n]+)",                  false},
    {"Headers Size", 2, "\nHeaders%sSize%s=%s([^\n]+)",           false},
    {"Tail Size",    2, "\nTail%sSize%s=%s([^\n]+)",              false},
    {"Blocks",       2, "\nBlocks%s=%s([^\n]+)",                  false},
    {"Streams",      2, "\nStreams%s=%s([^\n]+)",                 false},
    {"Comment",      8, "\nComment%s=%s([^\n]+)",                 false},
    {"Multivolume",  8, "\nMultivolume%s=%s([^\n]+)",             false},
    {"Volumes",      2, "\nVolumes%s=%s([^\n]+)",                 false},
    {"Code Page",    8, "\nCode%sPage%s=%s([^\n]+)",              false},
    {"Files",        2, "(%d+)%sfiles",                            true},
    {"Folders",      2, "(%d+)%sfolders",                          true},
    {"Size",         2, "(%d+)%s+%d+%s+%d+%sf",                    true},
    {"Compressed",   2, "(%d+)%s+%d+%sf",                          true},
    {"Date",         8, "%d%d%d%d%-%d%d%-%d%d%s%d%d:%d%d:%d%d",    true},
    {"Warnings",     2, "Warnings:%s(%d+)",                        true},
    {"Errors",       2, "Errors:%s(%d+)",                          true},
    {"Encrypted",    6, ":%sCan%snot%sopen%s(encrypted)%sarchive", true},
}

function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
  z7cmd = get_output("sh -c 'which 7zz || which 7z' 2>/dev/null"):sub(1, -2)
end

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2]
    end
    return '', '', 0 -- ft_nomorefields
end

function ContentGetDetectString()
    return 'EXT="7Z" | EXT="ZIP" | EXT="RAR" | EXT="XZ" | EXT="GZ" | EXT="Z" | EXT="LZMA" | EXT="BZ2" | EXT="TAR" | EXT="ZIPX"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (cached_file ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName)
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            output = ''
            return nil
        end
        output = get_output(z7cmd .. " l " .. FileName:gsub(' ', '\\ ') .. " -p 2>/dev/null")
        cached_file = FileName
    end

    if (output ~= '') then
        local target = output
        local result = ''
        if (fields[FieldIndex + 1][4] == true) then
            target = output:match("%-([%w%s%-:,]+)\n$")
        end
        if (target ~= nil) and (fields[FieldIndex + 1][3] ~= nil) then
            result = target:match(fields[FieldIndex + 1][3])
        else
            return nil
        end
        if (fields[FieldIndex + 1][2] == 6) then
            if (result == nil) then
                return false
            else
                return true
            end
        elseif (fields[FieldIndex + 1][2] == 2) and (result == nil) then
            if (FieldIndex == 12) or (FieldIndex == 13) then
                return 0
            end
        end
        return result
    end
    return nil -- invalid
end

function get_output(command)
  if not command then
    return ''
  end
  local handle = io.popen(command, 'r')
  local output = handle:read("*a")
  handle:close()
  return output
end