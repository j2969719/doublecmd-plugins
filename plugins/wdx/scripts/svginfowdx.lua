-- svginfowdx.lua (cross-platform)
-- 2019.12.24
--
-- Getting application name (this is not standardized!): see function GetApp() and table t.

local fields = {
 "SVG version",
 "Application",
 "xmlns-sodipodi",
 "xmlns-inkscape",
 "width",
 "height",
 "id",
 "sodipodi-version",
 "inkscape-version",
 "sodipodi-docbase",
 "sodipodi-docname",
 "viewBox",
 "baseProfile"
}
local fc
local filename = ''

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1], "", 8
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="SVG"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex > 12 then return nil end
  local at = SysUtils.FileGetAttr(FileName)
  if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
  local e, r
  if filename ~= FileName then
    e = string.lower(string.sub(FileName, string.len(FileName) - 3, -1))
    if e ~= '.svg' then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    local t = h:read('*a')
    h:close()
    if t == nil then return nil end
    local nb = string.find(t, '<svg', 1, true)
    local ne = string.find(t, '>', nb + 1, true)
    fc = string.sub(t, 1, ne)
    filename = FileName
  end
  if FieldIndex == 0 then
    r = GetAttr('version')
  elseif FieldIndex == 1 then
    r = GetApp()
  else
    r = GetAttr(fields[FieldIndex + 1])
  end
  return r
end

function GetAttr(an)
  local n = string.find(fc, '<svg', 1, true)
  local s = string.sub(fc, n, -1)
  an = string.gsub(an, '%-', ':')
  local r = string.match(s, '%s+' .. an .. '="([^"]+)"')
  return r
end

function GetApp()
  local n = string.find(fc, '<svg', 1, true)
  if n < 8 then return nil end
  local s = string.sub(fc, 1, n)
  local n1 = string.find(s, '<!--', 1, true)
  if n1 == nil then return nil end
  local n2 = string.find(s, '-->', n1, true)
  s = string.sub(s, n1, n2 - 1)
  local t = {'Created with', 'Creator:', 'Generator:'}
  local r
  for i = 1, #t do
    r = string.match(s, '<!%-%-%s*' .. t[i] .. '%s+(.+)')
    if r ~= nil then return string.gsub(r, '%s*$', '') end
  end
  return nil
end
