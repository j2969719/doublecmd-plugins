-- symlinkwdx.lua (cross-platform)
-- 2023.10.12
--[[
Returns the size of the symbolic link target (for files only!)
and some additional info.
]]

local fields = {
{"Size (x)",    "B|KB|MB|GB", 2},
{"Size (x.y)",  "B|KB|MB|GB", 3},
{"Size (x.yz)", "B|KB|MB|GB", 3},
{"Size (auto)", "x|x.y|x.yz", 8},
{"Target",      "object|object (recursive)", 8},
{"Target (parts)", "name|path|name (recursive)|path (recursive)", 8}
}
local aM = {
{1, " B"},
{1024, " KB"},
{1024 ^ 2, " MB"},
{1024 ^ 3, " GB"}
}
local aR = {"%.0f", "%.1f", "%.2f"}
local aP = {"", "", "", ""}

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
  local iattr = SysUtils.FileGetAttr(FileName)
  if (iattr < 0) or (math.floor(iattr / 0x00000400) % 2 == 0) then
    return nil
  end
  local st, t, n
  st = SysUtils.ReadSymbolicLink(FileName, true)
  if FieldIndex < 4 then
    if st == "" then return nil end
    if math.floor(iattr / 0x00000010) % 2 ~= 0 then
      return nil
    end
    n = GetFileSize(st)
    if n == nil then return nil end
    if FieldIndex == 3 then
      for i = #aM, #aM - 1, -1 do
        if n > aM[i][1] then
          t = string.format(aR[UnitIndex + 1], n / aM[i][1]) .. aM[i][2]
          break
        end
      end
      if t == nil then t = tostring(n) .. " B" end
      return t
    else
      t = string.format(aR[FieldIndex + 1], n / aM[UnitIndex + 1][1])
      return tonumber(t, 10)
    end
  elseif FieldIndex == 4 then
    if UnitIndex == 0 then
      return SysUtils.ReadSymbolicLink(FileName, false)
    elseif UnitIndex == 1 then
      if st == "" then
        return nil
      else
        return st
      end
    end
  elseif FieldIndex == 5 then
    if (UnitIndex == 0) or (UnitIndex == 1) then
      t = SysUtils.ReadSymbolicLink(FileName, false)
      aP[1] = SysUtils.ExtractFileName(t)
      aP[2] = SysUtils.ExtractFileDir(t)
    else
      aP[3] = SysUtils.ExtractFileName(st)
      aP[4] = SysUtils.ExtractFileDir(st)
    end
    if aP[UnitIndex + 1] == "" then
      return nil
    else
      return aP[UnitIndex + 1]
    end
  end
  return nil
end

function GetFileSize(s)
  local h, fd = SysUtils.FindFirst(s)
  if h ~= nil then
    SysUtils.FindClose(h)
    if math.floor(fd.Attr / 0x00000010) % 2 == 0 then
      return fd.Size
    end
  end
  return nil
end
