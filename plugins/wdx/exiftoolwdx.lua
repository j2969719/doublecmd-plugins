local fields = {
  {"File Type", 8},
  {"MIME Type", 8},
  {"Machine Type", 8},
  {"Time Stamp", 8},
  {"PE Type", 8},
  {"OS Version", 8},
  {"Subsystem", 8},
  {"Subsystem Version", 8},
  {"File Version Number", 8},
  {"Product Version Number", 8},
  {"Company Name", 8},
  {"File Description", 8},
  {"Internal Name", 8},
  {"File Version", 8},
  {"Comments", 8},
  {"Original File Name", 8},
  {"File OS", 8},
  {"Object File Type", 8},
  {"Language Code", 8},
  {"Character Set", 8},
  {"Legal Copyright", 8},
  {"Legal Trademarks", 8},
  {"Private Build", 8},
  {"Product Version", 8},
  {"Special Build", 8},
  {"Title", 8},
  {"Subject", 8},
  {"Author", 8},
  {"Keywords", 8},
  {"Last Modified By", 8},
  {"Create Date", 8},
  {"Modify Date", 8},
  {"Last Printed", 8},
  {"Revision Number", 8},
  {"Total Edit Time", 8},
  {"Pages", 2},
  {"Words", 2},
  {"Characters", 2},
  {"Lines", 2},
  {"Paragraphs", 2},
  {"Characters With Spaces", 2},
  {"Char Count With Spaces", 2},
  {"Internal Version Number", 2},
  {"Company", 8},
  {"Software", 8},
  {"App Version", 8},
  {"Code Page", 8},
  {"Title Of Parts", 8},
  {"Heading Pairs", 8},
  {"Security", 8},
  {"Scale Crop", 8},
  {"Links Up To Date", 8},
  {"Shared Doc", 8},
  {"Tag PID GUID", 8},
  {"Comp Obj User Type", 8},
  {"PDF Version", 8},
  {"Page Count", 2},
  {"Producer", 8},
  {"Creator", 8},
  {"JFIF Version", 8},
  {"GIF Version", 8},
  {"SVG Version", 8},
  {"Resolution Unit", 8},
  {"X Resolution", 2},
  {"Y Resolution", 2},
  {"Image Width", 2},
  {"Image Height", 2},
  {"Color Resolution Depth", 2},
  {"Bit Depth", 2},
  {"Bits Per Sample", 2},
  {"Color Components", 2},
  {"Frame Count", 2},
  {"Encoding Process", 8},
  {"Y Cb Cr Sub Sampling", 8},
  {"Image Size", 8},
  {"Megapixels", 8},
  {"Background Color", 8},
  {"Animation Iterations", 8},
  {"XMP Toolkit", 8},
  {"Creator Tool", 8},
  {"Has Color Map", 8},
  {"Instance ID", 8},
  {"Document ID", 8},
  {"Derived From Instance ID", 8},
  {"Derived From Document ID", 8},
  {"Color Type", 8},
  {"Compression", 8},
  {"Filter", 8},
  {"Interlace", 8},
  {"Pixel Units", 8},
  {"Pixels Per Unit X", 8},
  {"Pixels Per Unit Y", 8},
  {"Publisher", 8},
  {"Comment", 8},
  {"Xmlns", 8},
  {"Volume Name", 8},
  {"Volume Block Count", 2},
  {"Volume Block Size", 2},
  {"Root Directory Create Date", 8},
  {"Data Preparer", 8},
  {"Volume Create Date", 8},
  {"Volume Modify Date", 8},
  {"Boot System", 8},
  {"Volume Size", 8},
  {"Artist", 8},
  {"Album", 8},
  {"Genre", 8},
  {"Duration", 8},
  {"MPEG Audio Version", 2},
  {"Audio Layer", 2},
  {"Audio Bitrate", 8},
  {"Sample Rate", 2},
  {"Channel Mode", 8},
  {"MS Stereo", 8},
  {"Intensity Stereo", 8},
  {"Copyright Flag", 8},
  {"Original Media", 2},
  {"Emphasis", 8},
  {"ID3 Size", 8}
}

local filename = ""
local res = nil

function ContentGetSupportedField(Index)
  if (fields[Index+1] ~= nil ) then
    return fields[Index+1][1],"", fields[Index+1][2];
  end
  return '','', 0; -- ft_nomorefields
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if (SysUtils.DirectoryExists(FileName)) then return nil end;
  if (filename ~= FileName) then
    local handle = io.popen('exiftool "'..FileName..'"');
    res = handle:read("*a");
    handle:close();
    filename = FileName
  end
  local tmp = string.match(res, "\n"..fields[FieldIndex+1][1].."%s+:%s[^\n]+");
  if tmp ~= nil then
    return tmp:gsub("^\n"..fields[FieldIndex+1][1].."%s+:%s", "");
  end
  return nil; -- invalid
end
