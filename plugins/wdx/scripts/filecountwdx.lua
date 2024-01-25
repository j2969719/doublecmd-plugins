-- filecountwdx.lua (cross-platform)
-- 2024.01.25
--[[
Returns file or directory count and directory size (without scanning symbolic links to folders).
Only for columns set or tooltips!
]]

local fields = {
{"File count", "files|files and symlinks", 1},
{"Symlink count",      "", 1},
{"Folder count",       "", 1},
{"Folder size (x)",    "B|KB|MB|GB", 2},
{"Folder size (x.y)",  "KB|MB|GB",   3},
{"Folder size (x.yz)", "KB|MB|GB",   3},
{"Folder size (auto)", "x|x.y|x.yz", 8}
}
local aM = {
{1024, " KB"},
{1024 ^ 2, " MB"},
{1024 ^ 3, " GB"}
}
local aR = {"%.0f", "%.1f", "%.2f"}

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]
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
  if FieldIndex > 6 then return nil end
  if flags == 1 then return nil end; -- Исключаем вывод для диалога свойств (CONTENT_DELAYIFSLOW)
  local t = string.sub(FileName, string.len(FileName) - 3, -1)
  if t == SysUtils.PathDelim .. ".." then return nil end
  t = string.sub(t, 2, -1)
  if t == SysUtils.PathDelim .. "." then return nil end
  local k = SysUtils.FileGetAttr(FileName)
  if k < 0 then return nil end
  if (math.floor(k / 0x00000400) % 2 ~= 0) then return nil end
  if (math.floor(k / 0x00000010) % 2 ~= 0) then
    local nf, ns, nd, fs = ScanDir(FileName)
    if FieldIndex == 0 then
      if UnitIndex == 0 then
        return nf
      elseif UnitIndex == 1 then
        return nf + ns
      end
    elseif FieldIndex == 1 then
      return ns
    elseif FieldIndex == 2 then
      return nd
    elseif FieldIndex == 3 then
      if UnitIndex == 0 then
        return fs
      else
        t = string.format(aR[1], fs / aM[UnitIndex][1])
        return tonumber(t, 10)
      end
    elseif FieldIndex == 4 then
      t = string.format(aR[2], fs / aM[UnitIndex + 1][1])
      return tonumber(t, 10)
    elseif FieldIndex == 5 then
      t = string.format(aR[3], fs / aM[UnitIndex + 1][1])
      return tonumber(t, 10)
    elseif FieldIndex == 6 then
      t = ""
      for i = #aM, 1, -1 do
        if fs > aM[i][1] then
          t = string.format(aR[UnitIndex + 1], fs / aM[i][1]) .. aM[i][2]
          break
        end
      end
      if t == "" then t = tostring(fs) .. " B" end
      return t
    end
  end
  return nil
end

function ScanDir(p)
  local fc, sc, dc, fs = 0, 0, 0, 0
  local c1, c2, c3, c4
  local r = nil
  local h, d = SysUtils.FindFirst(p .. SysUtils.PathDelim .. "*")
  if h ~= nil then
    repeat
      if (d.Name ~= ".") and (d.Name ~= "..") then
        if (math.floor(d.Attr / 0x00000400) % 2 ~= 0) then
          sc = sc + 1
        else
          if (math.floor(d.Attr / 0x00000010) % 2 ~= 0) then
            dc = dc + 1
            c1, c2, c3, c4 = ScanDir(p .. SysUtils.PathDelim .. d.Name)
            fc = fc + c1
            sc = sc + c2
            dc = dc + c3
            fs = fs + c4
          else
            fc = fc + 1
            fs = fs + d.Size
          end
        end
      end
      r, d = SysUtils.FindNext(h)
    until r == nil
    SysUtils.FindClose(h)
  end
  return fc, sc, dc, fs
end
