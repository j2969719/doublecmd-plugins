-- hardlinkswdx.lua

function ContentGetSupportedField(Index)
  if (Index == 0) then
    return 'hardlinks','', 1; -- FieldName,Units,ft_string
  elseif (Index == 1) then
    return 'hardlinks(files only)','', 1;
  elseif (Index == 2) then
    return 'inode','', 1;
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
    local handle = io.popen('stat --printf=%h "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  elseif (FieldIndex == 1) then
    if (SysUtils.DirectoryExists(FileName)) then return nil end
    local handle = io.popen('stat --printf=%h "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  elseif (FieldIndex == 2) then
    local handle = io.popen('stat --printf=%i "'..FileName..'"');
    local result = handle:read("*a");
    handle:close();
    return result;
  end
  return nil; -- invalid
end
