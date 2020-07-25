-- getfattrcustomwdx.lua
-- 2020.07.26
--[[
Get custom extended attributes. Requires getfattr.
For Unix-like only!

"User defined attribute" returns value of custom extended attribute by name.
See (and edit) list of name these attributes in the "un" table.

"All extended attributes" returns list of all custom extended attributes
(will be separate by semicolon and space, i.e. "; ").
]]

local un = {
"comment",
"target",
"type"
}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "User defined attribute", table.concat(un, '|'), 8
  elseif FieldIndex == 1 then
    return "All extended attributes", "", 8
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 1 then return nil end
  local h, out
  if FieldIndex == 0 then
    if un[UnitIndex + 1] ~= nil then
      h = io.popen('getfattr --only-values -n user.' .. un[UnitIndex + 1] .. ' --absolute-names "' .. FileName .. '"')
      if h == nil then return nil end
      out = h:read('*a')
      h:close()
      if (out ~= nil) and (out ~= '') then
        out = string.gsub(out, '[\r\n]+$', '')
        if string.len(out) > 0 then return out end
      end
    end
  elseif FieldIndex == 1 then
    h = io.popen('getfattr -d --absolute-names "' .. FileName .. '"')
    if h == nil then return nil end
    out = h:read('*a')
    h:close()
    if (out ~= nil) and (out ~= '') then
      local s = ''
      for l in string.gmatch(out, 'user%.([^\r\n=]+)=') do
        s = s .. '; ' .. l
      end
      s = string.sub(s, 3, -1)
      if string.len(s) > 0 then return s end
    end
  end
  return nil
end
