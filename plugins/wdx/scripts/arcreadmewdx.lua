local units = '1'
local max_lines = 32
local line_len = 60
local z7cmd = '7z'

function ContentGetDetectString()
  return 'EXT="7Z" | EXT="ZIP" | EXT="RAR" | EXT="XZ" | EXT="GZ" | EXT="Z" | EXT="LZMA" | EXT="BZ2" | EXT="TAR" | EXT="ZIPX" | EXT="ZST"'
end

function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
  for i = 2, max_lines do
    units = units .. '|' .. i
  end
  z7cmd = get_output("sh -c 'which 7zz || which 7z' 2>/dev/null"):sub(1, -2)
end

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "readme lines", units, 8
  elseif FieldIndex == 1 then
    return "readme lines (skip empty)", units, 8
  end
  return '', '', 0 -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  local command = nil
  local prefix = "^Path = "
  local name = SysUtils.ExtractFileName(FileName)
  local tarball = name:find("%.tar%.?")
  if tarball then
    command = "tar -tf " .. FileName:gsub(' ', '\\ ') .. " 2>/dev/null"
    prefix = '^'
    name = name:gsub("%.tar.*", '')
  else
    command = z7cmd .. " l -slt " .. FileName:gsub(' ', '\\ ') .. " -p 2>/dev/null"
    name = name:gsub(SysUtils.ExtractFileExt(name), '')
  end
  local name_len = name:len()
  local output = get_output(command)
  if output == '' then
    return nil
  end
  local readme = nil
  for line in output:gmatch("([^\n]*)\n?") do
    local path = line:match(prefix .. "(.+)")
    if path ~= nil and (path:find("[Rr][Ee][Aa][Dd][Mm][Ee]") or path:find("FILE_ID.DIZ$")) then
      local _, sep = path:gsub('/', '')
      if sep == 0 or (sep == 1 and path:sub(1, name_len) == name) or (sep == 2 and path:sub(1, name_len + 2) == "./" .. name) then
        readme = path
        break
      end
    end
  end
  if readme then
    if tarball then
      command = "tar -xf " .. FileName:gsub(' ', '\\ ') .. ' ' .. readme:gsub(' ', '\\ ') .. " -O 2>/dev/null"
    else
      command = z7cmd .. " e " .. FileName:gsub(' ', '\\ ') .. ' ' .. readme:gsub(' ', '\\ ') .. " -so -y -- 2>/dev/null"
    end
    local count = 0
    output = get_output(command)
    enc = LazUtf8.DetectEncoding(output)
    if enc == "koi8r" then
      enc = "oem"
    end
    if enc ~= "utf8" then
      output = LazUtf8.ConvertEncoding(output, enc, "utf8")
    end
    local preview = ''
    for line in output:gmatch("([^\n\r]*)[\n\r]?") do
      if line == '' then
        if FieldIndex == 0 then
          line = ' '
        elseif FieldIndex == 1 then
          goto continue -- 10 PRINT "HELOWORLD"... :3
        end
      end
      preview = preview .. LazUtf8.Copy(line, 1, line_len) .. '\n'
      count = count + 1
      if count == UnitIndex + 1 then
        break
      end
      ::continue::
    end
    return preview
  end
  return nil
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

