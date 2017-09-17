-- duplicatewdx.lua

function ContentGetSupportedField(Index)
if (Index == 0) then
    return 'fast','', 6; -- FieldName,Units,ft_string
  elseif (Index == 1) then
    return 'force','', 6;
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
  local tmpfile = '/tmp/_dubsearchlst.log'
  local cmdp1 = 'find "'
  local cmdp2 = '" -type f -exec md5sum \'{}\' \';\' | sort | uniq --all-repeated=separate -w 33 | cut -c 35- > '
  local f=io.open(tmpfile,"r");
  if f~=nil then
    io.close(f);
    if (FieldIndex == 1) then os.execute(cmdp1..FileName:match("(.*/)")..cmdp2..tmpfile); end
  else
    os.execute(cmdp1..FileName:match("(.*/)")..cmdp2..tmpfile);
  end
  local f = io.open(tmpfile,"r");
  if not f then return nil end
  for line in f:lines() do
    if (FileName == line) then return true end
  end
  f:close();
  return false;
end
