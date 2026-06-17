local toplevel = nil
local last_dir = nil
local last_level = 0
local units = 'root'
for i = 1, 666 do
  units = units .. '|' .. i
end

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "level", units, 6
  elseif FieldIndex == 1 then
    return "level (subdirs is files)", units, 6
  elseif FieldIndex == 2 then
    return "reset toplevel", '', 6
  end
  return '','', 0 -- ft_nomorefields
end

function ContentGetDetectString()
  return 'EXT="*"' -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex < 2 then
    local dir = SysUtils.ExtractFileDir(FileName)
    if FieldIndex == 0 and SysUtils.DirectoryExists(FileName) then
      dir = FileName
    end
    if dir ~= last_dir then
      last_dir = dir
      last_level = get_level(dir)
    end
    return (last_level == UnitIndex)
  elseif FieldIndex == 2 then
    toplevel = nil
    last_dir = nil
    last_level = 0
  end
  return nil -- invalid
end

function get_level(path)
  local level = 0
  local dirs = {}
  for dir in path:gmatch("[^" .. SysUtils.PathDelim .. "]+") do
    table.insert(dirs, dir)
  end
  if toplevel == nil or #toplevel > #dirs then
    toplevel = dirs
    return level
  end
  for i = 1, #dirs do
    if i <= #toplevel and dirs[i] ~= toplevel[i] then
      for n = #toplevel, i, -1 do
        table.remove(toplevel, n)
      end
    elseif i > #toplevel then
      level = level + 1
    end
  end
  return level
end
