local z7cmd = '7z'
local pattern = "Comment = \n{\n(.+)}\n"
local notfoundstr = "not found"


function ContentGetDetectString()
  return 'EXT="7Z" | EXT="ZIP" | EXT="RAR"'
end

function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
  z7cmd = get_output("sh -c 'which 7zz || which 7z' 2>/dev/null"):sub(1, -2)
end

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return "comment", 'default|show "' .. notfoundstr .. '"', 8 -- FieldName,Units,ft_string
    end
    return '', '', 0 -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if not SysUtils.DirectoryExists(FileName) and (FieldIndex == 0) then
        local result = get_output(z7cmd .. " l " .. FileName:gsub(' ', '\\ ') .. " -p 2>/dev/null"):match(pattern)
        if (result ~= nil) then
            enc = LazUtf8.DetectEncoding(result)
            if enc == "koi8r" then
                enc = "oem"
            end
            if enc ~= "utf8" then
                result = LazUtf8.ConvertEncoding(result, enc, "utf8")
            end
        end
        if (UnitIndex == 1) and (result == nil) then
            return notfoundstr
        else
            return result
        end
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