-- poheaderwdx.lua (cross-platform)
-- 2020.08.23
--[[
Getting some information from PO-files (gettext):
  PO Translation Files and POT Translation Templates.

Symlinks will be ignored.

Supported fields: see table "fields".
]]

local fields = {
{"Project id/version",      "Project%-Id%-Version"},
{"Report bugs to",          "Report%-Msgid%-Bugs%-To"},
{"POT creation date",       "POT%-Creation%-Date"},
{"PO revision date",        "PO%-Revision%-Date"},
{"Last translator",         "Last%-Translator"},
{"Language team",           "Language%-Team"},
{"Language",                "Language"},
{"X-Generator",             "X%-Generator"},
{"X-Language",              "X%-Language"},
{"X-Native-Language",       "X%-Native%-Language"},
{"X-Source-Language",       "X%-Source%-Language"},
{"X-Poedit-Language",       "X%-Poedit%-Language"},
{"X-Poedit-Country",        "X%-Poedit%-Country"},
{"X-Launchpad-Export-Date", "X%-Launchpad%-Export%-Date"},
{"X-Crowdin-Project",       "X%-Crowdin%-Project"},
{"X-Crowdin-Language",      "X%-Crowdin%-Language"}
}
local all = {}
local filename = ''

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], "", 8
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return '(EXT="PO")|(EXT="POT")'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) or (math.floor(at / 0x00000400) % 2 ~= 0) then return nil end
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if (e ~= '.po') and (e ~= '.pot') then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    local t, nb, ne
    local b = false
    local s = h:read(10240)
    if s ~= nil then
      nb = string.find(s, 'msgid "', 1, true)
      if nb == nil then
        local c = 10240
        while true do
          t = h:read(10240)
          if t == nil then break end
          s = s .. t
          nb = string.find(s, 'msgid "', c - 7, true)
          if nb ~= nil then break end
          c = c + 10240
        end
      end
      if nb ~= nil then
        ne = string.find(s, 'msgid "', nb + 7, true)
        if ne == nil then
          while true do
            t = h:read(5120)
            if t == nil then break end
            s = s .. t
            ne = string.find(s, 'msgid "', c - 7, true)
            if ne ~= nil then
              b = true
              break
            end
            c = c + 5120
          end
        else
          b = true
        end
      end
    end
    h:close()
    if b == false then return nil end
    for i = 1, #all do all[i] = nil end
    t = string.sub(s, nb, ne)
    for i = 1, #fields do
      nb, ne = string.find(t, '[\r\n]"' .. fields[i][2] .. ': *', 1, false)
      if nb == nil then
        all[i] = ''
      else
        nb = string.find(t, '\\n"', ne, true)
        if nb == nil then
          all[i] = ''
        else
          s = string.sub(t, ne + 1, nb - 1)
          if string.find(s, '"[\r\n]+"', 1, false) ~= nil then
            all[i] = string.gsub(s, '"[\r\n]+"', '')
          else
            all[i] = s
          end
        end
      end
    end
    filename = FileName
  end
  if all[FieldIndex + 1] == '' then
    return nil
  else
    return all[FieldIndex + 1]
  end
end
