-- folderbytextwdx.lua (cross-platform)
-- 2024.07.30
--[[
Search for folders by contents of text files.
Without a recursive search, i.e. for file search tool only.

Symbolic links to directories will be ignored.
List of extensions: see the "aExt" table.
List of supported encodings: see LazUtf8.ConvertEncoding
  https://doublecmd.github.io/doc/en/lua.html#libraryutf8
]]

local aExt = {"txt", "cue"}
local sRet = ""

function ContentGetSupportedField(FieldIndex)
  if aExt[FieldIndex + 1] ~= nil then
    return "Folder with " .. aExt[FieldIndex + 1], "", 9
  end
  return "", "", 0
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if aExt[FieldIndex + 1] == nil then return nil end
  if flags == 1 then return nil end; -- CONTENT_DELAYIFSLOW, https://doublecmd.github.io/doc/en/lua.html#preface
  if UnitIndex ~= 0 then return nil end
  local k = SysUtils.FileGetAttr(FileName)
  if k < 0 then return nil end
  if (math.floor(k / 0x00000400) % 2 ~= 0) then return nil end
  if (math.floor(k / 0x00000010) % 2 ~= 0) then
    sRet = ""
    local r = nil
    local h, d = SysUtils.FindFirst(FileName .. SysUtils.PathDelim .. "*." .. aExt[FieldIndex + 1])
    if h ~= nil then
      repeat
        if (math.floor(d.Attr / 0x00000010) % 2 == 0) then
          if d.Size > 0 then GetText(FileName .. SysUtils.PathDelim .. d.Name, d.Size) end
        end
        r, d = SysUtils.FindNext(h)
      until r == nil
      SysUtils.FindClose(h)
    end
    if sRet == "" then return nil else return sRet end
  end
  return nil
end

function GetText(p, n)
  local h = io.open(p, "rb")
  if h == nil then return nil end
  local cf = h:read("*a")
  h:close()
  local t, e
  if n > 4096 then
    e = LazUtf8.DetectEncoding(string.sub(cf, 1, 4096))
  else
    e = LazUtf8.DetectEncoding(cf)
  end
  if e == "utf8" then
    sRet = sRet .. "\n" .. cf
  elseif e == "utf8bom" then
    sRet = sRet .. "\n" .. string.sub(cf, 4, -1)
  else
    t = LazUtf8.ConvertEncoding(cf, e, "utf8")
    sRet = sRet .. "\n" .. t
  end
end
