-- groupswdx.lua (cross-platform)
--2023.07.16
--[[
Returns the group name by the file mask
(see/edit the "aGroups" table below).
]]

local aGroups = {
{"Report",   "report_*"},
{"Executable (Win)", "*.bat;*.cmd;*.com;*.exe;*.js;*.msi;*.msu;*.vbs;*.wsf"},
{"Document", "*.chm;*.djvu;*.doc;*.docx;*.epub;*.odp;*.ods;*.odt;*.pdf;*.ppt;*.pptx;*.xls;*.xlsx"},
{"Image",    "*.avif;*.bmp;*.gif;*.heic;*.heif;*.jpg;*.jpeg;*.png;*.psd;*.tif;*.tiff;*.webp"},
{"Video",    "*.avi;*.mpg;*.mp4;*.mov;*.mkv;*.wmv"}
}

function ContentGetSupportedField(FieldIndex)
  if FieldIndex == 0 then
    return "Group", "", 8; -- FieldName, Units, ft_string
  end
  return "", "", 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1
end

function ContentGetDetectString()
  return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex == 0 then
    local bResult, sResult
    local sName = SysUtils.ExtractFileName(FileName)
    local iCount = 0
    while true do
      iCount = iCount + 1
      bResult = SysUtils.MatchesMaskList(sName, aGroups[iCount][2], ";", 0)
      if bResult == true then
        sResult = aGroups[iCount][1]
        break
      end
      if iCount == #aGroups then break end
    end
    return sResult
  end
  return nil
end
