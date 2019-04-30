-- fb2isvalidwdx.lua (Unix-like only)
-- 2019.04.30

local p

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Is valid", "well-formedness only|XSD schema FB 2.0|XSD schema FB 2.1|XSD schema FB 2.2", 6
  end
  return "", "", 0
end

function ContentGetDetectString()
  return 'EXT="FB2"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 0 then return nil end
  local ps
  if UnitIndex == 0 then
    ps  = '--well-formed'
  else
    if p == nil then
      p = debug.getinfo(1).short_src
      local i, j = string.find(s, '/.*/')
      if i == nil then return nil end
      p = string.sub(p, i, j) .. 'fb2-xsd/'
    end
    local v
    if UnitIndex == 1 then
      v = '2.0'
    elseif UnitIndex == 2 then
      v = '2.1'
    elseif UnitIndex == 3 then
      v = '2.2'
    else
      return nil
    end
    ps = '--xsd "' .. p .. v .. '/FictionBook.xsd"'
  end
  local h = io.popen('xmlstarlet val ' .. ps .. ' "' .. FileName .. '"')
  if not h then
    -- FreeBSD, some Linux distributions
    h = io.popen('xml val ' .. ps .. ' "' .. FileName .. '"')
    if not h then return nil end
  end
  local o = h:read("*a")
  h:close()
  o = string.sub(o, -8, -2)
  if o == '- valid' then
    return true
  elseif o == 'invalid' then
    return false
  end
  return nil
end
