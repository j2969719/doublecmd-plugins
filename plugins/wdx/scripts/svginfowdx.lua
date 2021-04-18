-- svginfowdx.lua (cross-platform)
-- 2021.04.18
--[[
Getting some information from SVG files.
Similar to SVGInfo by Progman13 (https://totalcmd.net/plugring/svginfo.html).

SVGZ support: needs lua-zlib module (https://github.com/brimworks/lua-zlib).

About "Calculated *, X DPI" fields:
returns size in pixels and value depends on DPI, but
- if "width" and "height" in pixels:
  DPI will be ignored and all this fields returns values as is, without calculation;
- if "width" and "height" are empty or size in em, ex or percents:
  DPI will be ignored and all this fields returns values from "viewBox" (if exists) as is, without calculation.

Note: Will read first 100 KB (for speed  up).
]]

local rz, zlib = pcall(require, 'zlib')

local fields = {
{"Application",      "",                 8},
{"SVG version",      "version",          8},
{"Width",            "width",            8},
{"Height",           "height",           8},
{"ViewBox",          "viewBox",          8},
{"ID",               "id",               8},
{"Inkscape version", "inkscape:version", 8},
{"Sodipodi version", "sodipodi:version", 8},
{"Sodipodi docbase", "sodipodi:docbase", 8},
{"Sodipodi docname", "sodipodi:docname", 8},
{"Calculated width, 96 DPI",  "", 1},
{"Calculated width, 90 DPI",  "", 1},
{"Calculated width, 72 DPI",  "", 1},
{"Calculated height, 96 DPI", "", 1},
{"Calculated height, 90 DPI", "", 1},
{"Calculated height, 72 DPI", "", 1}
}
local ag = {"Created with", "Creator:", "Generator:"}
local ar = {}
local filename = ''

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][3]
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return '(EXT="SVG")|(EXT="SVGZ")'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    local buf
    if e == '.svg' then
      buf = 102400
    elseif e == '.svgz' then
      if rz == true then buf = '*a' else return nil end
    else
      return nil
    end
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
    local h = io.open(FileName, 'rb')
    if h == nil then return nil end
    local t = h:read(buf)
    h:close()
    if t == nil then return nil end
    if e == '.svgz' then
      local stream = zlib.inflate()
      local unt = stream(t, 'finish')
      if unt == nil then return nil else t = unt end
    end
    local nb = string.find(t, '<svg', 1, true)
    if nb == nil then return nil end
    local ne = string.find(t, '>', nb + 1, true)
    local fc = string.sub(t, 1, ne)
    for i = 1, 16 do ar[i] = '' end
    local r
    local n1 = string.find(fc, '<!--', 1, true)
    if n1 ~= nil then
      local n2 = string.find(fc, '-->', n1, true)
      local s = string.sub(fc, n1, n2 - 1)
      for i = 1, #ag do
        r = string.match(s, '<!%-%-%s*' .. ag[i] .. '%s+(.+)')
        if r ~= nil then
          ar[1] = string.gsub(r, '%s*$', '')
          break
        end
      end
    end
    for i = 2, 10 do
      r = string.match(fc, '%s+' .. fields[i][2] .. '="([^"]+)"', nb)
      if r ~= nil then ar[i] = r end
    end
    WHCalculation()
    filename = FileName
  end
  if ar[FieldIndex + 1] == '' then
    return nil
  else
    return ar[FieldIndex + 1]
  end
end

function WHCalculation()
  if ar[3] == '' then
    GetFromViewBox()
    return 1
  end
  local cw = tonumber(string.match(ar[3], '^([0-9%.]+)'))
  local ch = tonumber(string.match(ar[4], '^([0-9%.]+)'))
  local w = string.match(ar[3], '([a-z]+)$')
  if (w == nil) or (w == 'px') then
    ar[11] = RoundU(cw)
    ar[12] = ar[11]
    ar[13] = ar[11]
    ar[14] = RoundU(ch)
    ar[15] = ar[14]
    ar[16] = ar[14]
  elseif w == 'pt' then
    ar[11] = RoundU(cw / 72 * 96)
    ar[12] = RoundU(cw / 72 * 90)
    ar[13] = RoundU(cw / 72 * 72)
    ar[14] = RoundU(ch / 72 * 96)
    ar[15] = RoundU(ch / 72 * 90)
    ar[16] = RoundU(ch / 72 * 72)
  elseif w == 'pc' then
    ar[11] = RoundU(cw * (16 / 96 * 96))
    ar[12] = RoundU(cw * (16 / 96 * 90))
    ar[13] = RoundU(cw * (16 / 96 * 72))
    ar[14] = RoundU(ch * (16 / 96 * 96))
    ar[15] = RoundU(ch * (16 / 96 * 90))
    ar[16] = RoundU(ch * (16 / 96 * 72))
  elseif w == 'in' then
    ar[11] = RoundU(cw * 96)
    ar[12] = RoundU(cw * 90)
    ar[13] = RoundU(cw * 72)
    ar[14] = RoundU(ch * 96)
    ar[15] = RoundU(ch * 90)
    ar[16] = RoundU(ch * 72)
  elseif w == 'cm' then
    ar[11] = RoundU(cw / 2.54 * 96)
    ar[12] = RoundU(cw / 2.54 * 90)
    ar[13] = RoundU(cw / 2.54 * 72)
    ar[14] = RoundU(ch / 2.54 * 96)
    ar[15] = RoundU(ch / 2.54 * 90)
    ar[16] = RoundU(ch / 2.54 * 72)
  elseif w == 'mm' then
    ar[11] = RoundU(cw / 25.4 * 96)
    ar[12] = RoundU(cw / 25.4 * 90)
    ar[13] = RoundU(cw / 25.4 * 72)
    ar[14] = RoundU(ch / 25.4 * 96)
    ar[15] = RoundU(ch / 25.4 * 90)
    ar[16] = RoundU(ch / 25.4 * 72)
  elseif (w == 'em') or (w == 'ex') then
    GetFromViewBox()
  end
end

function GetFromViewBox()
  if ar[5] == '' then return nil end
  local cw, ch = string.match(ar[5], '[0-9%.]+ [0-9%.]+ ([0-9%.]+) ([0-9%.]+)')
  if cw == nil then return nil end
  ar[11] = RoundU(tonumber(cw))
  ar[12] = ar[11]
  ar[13] = ar[11]
  ar[14] = RoundU(tonumber(ch))
  ar[15] = ar[14]
  ar[16] = ar[14]
end

function RoundU(n)
  return math.floor(n + 0.5)
end
