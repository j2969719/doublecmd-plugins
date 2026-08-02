-- mp3tagswdx.lua (cross-platform)
-- 2026.08.01
--[[
Check the existence of tags (ID3v1/ID3v2/APEv2) in MP3 files.
]]

local fields = {
{"ID3v1", 6},
{"ID3v2", 6},
{"APEv2", 6}
}
local ar = {false, false, false}
local filename = ""

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2]
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="MP3"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if e ~= ".mp3" then return nil end
    local at = SysUtils.FileGetAttr(FileName)
    if at < 0 then return nil end
    if math.floor(at / 0x00000010) % 2 ~= 0 then return nil end
    local h = io.open(FileName, "rb")
    if h == nil then return nil end
    for i = 1, #fields do ar[i] = false end
    -- ID3v2
    local d = h:read(3)
    if d == "ID3" then
      ar[2] = true
    else
      -- MP3 without ID3v2?
      if BinToNumBE(d, 1, 2) ~= 0xfffb then
        h:close()
        return nil
      end
    end
    -- Get file size
    at = h:seek("end")
    -- APEv2
    -- APEv2 only?
    h:seek("set", at - 32)
    d = h:read(16)
    if string.sub(d, 1, 8) == "APETAGEX" then
      ar[3] = true
    else
      -- APEv2 + ID3v1?
      h:seek("set", at - 160)
      d = h:read(16)
      if string.sub(d, 1, 8) == "APETAGEX" then
        ar[3] = true
      end
      -- ID3v1
      h:seek("set", at - 128)
      d = h:read(3)
      if d == "TAG" then
        ar[1] = true
      end
    end
    h:close()
    filename = FileName
  end
  return ar[FieldIndex + 1]
end
