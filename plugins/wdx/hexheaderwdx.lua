-- hexheaderwdx.lua

function ContentGetSupportedField(Index)
if (Index == 0) then
    return '1 byte','', 8; -- FieldName,Units,ft_string
  elseif (Index == 1) then
    return '2 bytes','', 8;
  elseif (Index == 2) then
    return '3 bytes','', 8;
  elseif (Index == 3) then
    return '4 bytes','', 8;
  elseif (Index == 4) then
    return '5 bytes','', 8;
  elseif (Index == 5) then
    return '6 bytes','', 8;
  elseif (Index == 6) then
    return '7 bytes','', 8;
  elseif (Index == 7) then
    return '8 bytes','', 8
  elseif (Index == 8) then
    return '9 bytes','', 8;
  elseif (Index == 9) then
    return '10 bytes','', 8;
  elseif (Index == 10) then
    return 'ALL FILE!!!!111!!','', 9;
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
  local f = assert(io.open(FileName, "rb"));
  if (FieldIndex < 10) then
    bytes = f:read(FieldIndex+1);
  else
    bytes = f:read("*a");
  end
  if (bytes == nil) then
    return nil; -- invalid
  end
  local t = {}
  for b in string.gfind(bytes, ".") do
    table.insert(t, string.format("%02X ", string.byte(b)));
  end
  local result = table.concat(t);
  local result = result:sub(1, -2);
  return result;
end
