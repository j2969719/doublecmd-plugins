-- Get SELinux file labels
-- 2019.01.21

function ContentGetSupportedField(FieldIndex)
  local fields = {
{"All",   8},
{"User",  8},
{"Role",  8},
{"Type",  8},
{"Level", 8}
  }
  if (fields[FieldIndex + 1] ~= nil) then
    return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2];
  end
  return "", "", 0; -- ft_nomorefields
end

function ContentGetDetectString()
  return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if (FieldIndex > 4) then return nil end;
  local h = io.popen('ls -Z "' .. FileName:gsub('"', '\\"') .. '"', 'r')
  if not h then return nil end;
  local s = h:read("*a")
  h:close();
  if (s == nil) then return nil end;
  s = string.match(s, "([a-z0-9_]+:[a-z0-9_]+:[a-z0-9_]+:s%d+)");
  if (s == nil) then return nil end;
  if (FieldIndex == 0) then return s end;
  local n1 = string.find(s, ':', 1, true)
  if (FieldIndex == 1) then return string.sub(s, 1, n1 - 1) end;
  local n2 = string.find(s, ':', n1 + 1, true)
  if (FieldIndex == 2) then return string.sub(s, n1 + 1, n2 - 1) end;
  n1 = string.find(s, ':', n2 + 1, true);
  if (FieldIndex == 3) then return string.sub(s, n2 + 1, n1 - 1) end;
  if (FieldIndex == 4) then return string.sub(s, n1 + 1, -1) end;
  return nil;
end
