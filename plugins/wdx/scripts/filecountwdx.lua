-- filecountwdx.lua (cross-platform)
-- 2019.08.09
--[[
Returns file or directory count, without scanning symbolic links to folders.
Only for columns set or tooltips!
]]

local fields = {
 {"File count", "files|files and symlinks"},
 {"Symlink count", ""},
 {"Folder count", ""}
}

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][2], 1
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
  if FieldIndex > 2 then return nil end
  if flags == 1 then return nil end; -- Исключаем вывод для диалога свойств (CONTENT_DELAYIFSLOW)
  local t = string.sub(FileName, string.len(FileName) - 3, -1)
  if t == SysUtils.PathDelim .. ".." then return nil end
  t = string.sub(t, 2, -1)
  if t == SysUtils.PathDelim .. "." then return nil end
  local k, nf, ns, nd
  k = SysUtils.FileGetAttr(FileName)
  if k > 0 then
    if (math.floor(k / 0x00000400) % 2 ~= 0) then return nil end
    if (math.floor(k / 0x00000010) % 2 ~= 0) then
      nf, ns, nd = ScanDir(FileName)
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
      end
    end
  end
  return nil
end

function ScanDir(p)
  local fc, sc, dc = 0, 0, 0
  local c1, c2, c3
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
            c1, c2, c3 = ScanDir(p .. SysUtils.PathDelim .. d.Name)
            fc = fc + c1
            sc = sc + c2
            dc = dc + c3
          else
            fc = fc + 1
          end
        end
      end
      r, d = SysUtils.FindNext(h)
    until r == nil
    SysUtils.FindClose(h)
  end
  return fc, sc, dc
end
