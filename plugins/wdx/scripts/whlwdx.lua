local z7cmd = "7z"
local output = ''
local cached_file = ''
local pipe_attr = 0x00000004

local fields = {
  {"Name",            8, "Name:",                                        false},
  {"Version",         8, "Version:",                                     false},
  {"Summary",         8, "Summary:",                                     false},
  {"Homepage",        8, "Home-page:",                                   false},
  {"Author",          8, "Author:",                                      false},
  {"Author-email",    8, "Author-email:",                                false},
  {"License",         8, "License:",                                     false},
  {"Keywords",        8, "Keywords:",                                    false},
  {"Requires-Python", 8, "Requires-Python:",                             false},
  {"Python",          8, "Classifier: Programming Language :: Python ::", true},
  {"Requires-Dist",   8, "Requires-Dist:",                                true},
}

local function hasattr(fileattr, value)
  return (fileattr > 0 and math.floor(fileattr / value) % 2 ~= 0)
end

local function get_output(command)
  if not command then
    return ''
  end
  local handle = io.popen(command, 'r')
  local output = handle:read("*a")
  handle:close()
  return output
end

function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
  z7cmd = get_output("sh -c 'which 7zz || which 7z' 2>/dev/null"):sub(1, -2)
  if (z7cmd == '') then
    plug_name = debug.getinfo(1).source:match("/([^/]+)$")
    DC.LogWrite(plug_name .. ": 7zip is not installed", 2, true, false)
  end
end

function ContentGetSupportedField(FieldIndex)
  if (fields[FieldIndex + 1] ~= nil) then
    return fields[FieldIndex + 1][1], '', fields[FieldIndex + 1][2]
  end
  return '', '', 0 -- ft_nomorefields
end

function ContentGetDetectString()
  return 'EXT="WHL"' -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if (z7cmd == '') then
    return nil
  end
  if (cached_file ~= FileName) then
    local attr = SysUtils.FileGetAttr(FileName)
    if hasattr(attr, pipe_attr) then
      output = ''
      return nil
    end
    output = get_output(z7cmd .. ' e ' .. FileName:gsub(' ', '\\ ') .. ' "*.dist-info/METADATA" -so -p 2>/dev/null')
    cached_file = FileName
  end
  if (output ~= '') then
    local pattern = "\n" .. fields[FieldIndex + 1][3]:gsub("([%%%.-%+%*%?%^%$%(%)%[%]])", "%%%1") .. " ([^\n]+)"
    if (fields[FieldIndex + 1][4] == true) then
      local result = {}
      for line in output:gmatch(pattern) do
        table.insert(result, line)
      end
      return table.concat(result, ", ")
    else
      return output:match(pattern)
    end
  end
  return nil -- invalid
end

