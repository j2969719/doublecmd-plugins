-- simplefileinfo.lua

function ContentGetSupportedField(Index)
  if (Index == 0) then
    return 'File type (file -b)','', 8; -- FieldName,Units,ft_string
  elseif (Index == 1) then
    return 'Access rights in octal (stat -c%a)','', 1;
  elseif (Index == 2) then
    return 'Access rights in text (stat -c%A)','', 8;
  elseif (Index == 3) then
    return 'User ID (stat -c%u)','', 1;
  elseif (Index == 4) then
    return 'User name (stat -c%U)','', 8;
  elseif (Index == 5) then
    return 'Group ID (stat -c%g)','', 1;
  elseif (Index == 6) then
    return 'Group name (stat -c%G)','', 8;
  elseif (Index == 7) then
    return 'CRC32','', 8;
  elseif (Index == 8) then
    return 'MD5','', 8;
  elseif (Index == 9) then
    return 'SHA256','', 8;
  end
  return '','', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if (string.sub(FileName, -3) == "/..") or (string.sub(FileName, -2) == "/.") then
    return nil;
  end
  if (FieldIndex == 0) then
    local handle = io.popen('file -b "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    local result = result:sub(1, -2);
    return result;
  elseif (FieldIndex == 1) then
    local handle = io.popen('stat --printf=%a "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  elseif (FieldIndex == 2) then
    local handle = io.popen('stat --printf=%A "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  elseif (FieldIndex == 3) then
    local handle = io.popen('stat --printf=%u "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  elseif (FieldIndex == 4) then
    local handle = io.popen('stat --printf=%U "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  elseif (FieldIndex == 5) then
    local handle = io.popen('stat --printf=%g "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  elseif (FieldIndex == 6) then
    local handle = io.popen('stat --printf=%G "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  elseif (FieldIndex == 7) then
    local handle = io.popen('crc32 "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    local result = result:sub(1, -2);
    return result;
  elseif (FieldIndex == 8) then
    local handle = io.popen('md5sum "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    local result = string.match(result, '(%w+)%s+');
    return result;
  elseif (FieldIndex == 9) then
    local handle = io.popen('sha256sum "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    local result = string.match(result, '(%w+)%s+');
    return result;
  end
  return nil; -- invalid
end
