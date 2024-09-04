-- filecountbyextwdx.lua (cross-platform)
-- 2024.07.18
--[[
Returns the number of files of the specified type (symbolic links to directories will be ignored).
No recursive search, for file search tool only!
List of extensions: see the "aExt" table.
]]

local aExt = {"png", "svg"}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Number of files", table.concat(aExt, "|"), 1
  end
  return "", "", 0
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex ~= 0 then return nil end
  if flags == 1 then return nil end; -- CONTENT_DELAYIFSLOW, https://doublecmd.github.io/doc/en/lua.html#preface
  local k = SysUtils.FileGetAttr(FileName)
  if k < 0 then return nil end
  if (math.floor(k / 0x00000400) % 2 ~= 0) then return nil end
  if (math.floor(k / 0x00000010) % 2 ~= 0) then
    local n = 0
    local r = nil
    local h, d = SysUtils.FindFirst(FileName .. SysUtils.PathDelim .. "*." .. aExt[UnitIndex + 1])
    if h ~= nil then
      repeat
        if (math.floor(d.Attr / 0x00000010) % 2 == 0) then
          n = n + 1
        end
        r, d = SysUtils.FindNext(h)
      until r == nil
      SysUtils.FindClose(h)
    end
    return n
  end
  return nil
end
