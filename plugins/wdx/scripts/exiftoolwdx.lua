-- exiftoolwdx.lua

local cmd = "exiftool"

local ft_string = 8
local ft_number = 2
local ft_float  = 3

local fields = {  -- pattern (also used as a field name if it is not specified), field type, field name
    {"Machine Type",                                                ft_string,                                nil}, 
    {"Time Stamp",                                                  ft_string,                                nil}, 
    {"PE Type",                                                     ft_string,                                nil}, 
    {"OS Version",                                                  ft_float,                                 nil}, 
    {"Subsystem",                                                   ft_string,                                nil}, 
    {"Subsystem Version",                                           ft_string,                                nil}, 
    {"File Version Number",                                         ft_string,                                nil}, 
    {"Product Name",                                                ft_string,                                nil}, 
    {"Product Version Number",                                      ft_string,                                nil}, 
    {"Company Name",                                                ft_string,                                nil}, 
    {"File Description",                                            ft_string,                                nil}, 
    {"Internal Name",                                               ft_string,                                nil}, 
    {"File Version",                                                ft_string,                                nil}, 
    {"Comments",                                                    ft_string,                                nil}, 
    {"Original File Name",                                          ft_string,                                nil}, 
    {"File OS",                                                     ft_string,                                nil}, 
    {"Object File Type",                                            ft_string,                                nil}, 
    {"CPU Architecture",                                            ft_string,                                nil}, 
    {"CPU Type",                                                    ft_string,                                nil}, 
    {"CPU Byte Order",                                              ft_string,                                nil}, 
    {"File Subtype",                                                ft_string,                                nil}, 
    {"File Flags",                                                  ft_string,                                nil}, 
    {"File Flags Mask",                                             ft_string,                                nil}, 
    {"Linker Version",                                              ft_string,                                nil}, 
    {"Code Size",                                                   ft_string,                                nil}, 
    {"Initialized Data Size",                                       ft_string,                                nil}, 
    {"Uninitialized Data Size",                                     ft_string,                                nil}, 
    {"Entry Point",                                                 ft_string,                                nil}, 
    {"Language Code",                                               ft_string,                                nil}, 
    {"Character Set",                                               ft_string,                                nil}, 
    {"Legal Copyright",                                             ft_string,                                nil}, 
    {"Legal Trademarks",                                            ft_string,                                nil}, 
    {"Private Build",                                               ft_string,                                nil}, 
    {"Product Version",                                             ft_string,                                nil}, 
    {"Special Build",                                               ft_string,                                nil}, 
    {"Image Version",                                               ft_string,                                nil}, 
    {"Title",                                                       ft_string,                                nil}, 
    {"Subject",                                                     ft_string,                                nil}, 
    {"Author",                                                      ft_string,                                nil}, 
    {"Keywords",                                                    ft_string,                                nil}, 
    {"Last Modified By",                                            ft_string,                                nil}, 
    {"Create Date",                                                 ft_string,                                nil}, 
    {"Modify Date",                                                 ft_string,                                nil}, 
    {"Last Printed",                                                ft_string,                                nil}, 
    {"Revision Number",                                             ft_string,                                nil}, 
    {"Total Edit Time",                                             ft_string,                                nil}, 
    {"Pages",                                                       ft_number,                                nil}, 
    {"Words",                                                       ft_number,                                nil}, 
    {"Characters",                                                  ft_number,                                nil}, 
    {"Lines",                                                       ft_number,                                nil}, 
    {"Paragraphs",                                                  ft_number,                                nil}, 
    {"Characters With Spaces",                                      ft_number,                                nil}, 
    {"Char Count With Spaces",                                      ft_number,                                nil}, 
    {"Internal Version Number",                                     ft_number,                                nil}, 
    {"Company",                                                     ft_string,                                nil}, 
    {"Software",                                                    ft_string,                                nil}, 
    {"App Version",                                                 ft_string,                                nil}, 
    {"Code Page",                                                   ft_string,                                nil}, 
    {"Title Of Parts",                                              ft_string,                                nil}, 
    {"Heading Pairs",                                               ft_string,                                nil}, 
    {"Security",                                                    ft_string,                                nil}, 
    {"Scale Crop",                                                  ft_string,                                nil}, 
    {"Links Up To Date",                                            ft_string,                                nil}, 
    {"Shared Doc",                                                  ft_string,                                nil}, 
    {"Tag PID GUID",                                                ft_string,                                nil}, 
    {"Comp Obj User Type",                                          ft_string,                                nil}, 
    {"PDF Version",                                                 ft_string,                                nil}, 
    {"Page Count",                                                  ft_number,                                nil}, 
    {"Producer",                                                    ft_string,                                nil}, 
    {"Creator",                                                     ft_string,                                nil}, 
    {"JFIF Version",                                                ft_string,                                nil}, 
    {"GIF Version",                                                 ft_string,                                nil}, 
    {"SVG Version",                                                 ft_string,                                nil}, 
    {"Resolution Unit",                                             ft_string,                                nil}, 
    {"X Resolution",                                                ft_number,                                nil}, 
    {"Y Resolution",                                                ft_number,                                nil}, 
    {"Image Width",                                                 ft_number,                                nil}, 
    {"Image Height",                                                ft_number,                                nil}, 
    {"Color Resolution Depth",                                      ft_number,                                nil}, 
    {"Bit Depth",                                                   ft_number,                                nil}, 
    {"Bits Per Sample",                                             ft_number,                                nil}, 
    {"Color Components",                                            ft_number,                                nil}, 
    {"Frame Count",                                                 ft_number,                                nil}, 
    {"Encoding Process",                                            ft_string,                                nil}, 
    {"Y Cb Cr Sub Sampling",                                        ft_string,                                nil}, 
    {"Image Size",                                                  ft_string,                                nil}, 
    {"Megapixels",                                                  ft_string,                                nil}, 
    {"Background Color",                                            ft_string,                                nil}, 
    {"Animation Iterations",                                        ft_string,                                nil}, 
    {"XMP Toolkit",                                                 ft_string,                                nil}, 
    {"Creator Tool",                                                ft_string,                                nil}, 
    {"Has Color Map",                                               ft_string,                                nil}, 
    {"Instance ID",                                                 ft_string,                                nil}, 
    {"Document ID",                                                 ft_string,                                nil}, 
    {"Derived From Instance ID",                                    ft_string,                                nil}, 
    {"Derived From Document ID",                                    ft_string,                                nil}, 
    {"Color Type",                                                  ft_string,                                nil}, 
    {"Compression",                                                 ft_string,                                nil}, 
    {"Filter",                                                      ft_string,                                nil}, 
    {"Interlace",                                                   ft_string,                                nil}, 
    {"Pixel Units",                                                 ft_string,                                nil}, 
    {"Pixels Per Unit X",                                           ft_string,                                nil}, 
    {"Pixels Per Unit Y",                                           ft_string,                                nil}, 
    {"Publisher",                                                   ft_string,                                nil}, 
    {"Comment",                                                     ft_string,                                nil}, 
    {"Xmlns",                                                       ft_string,                                nil}, 
    {"Volume Name",                                                 ft_string,                                nil}, 
    {"Volume Block Count",                                          ft_number,                                nil}, 
    {"Volume Block Size",                                           ft_number,                                nil}, 
    {"Root Directory Create Date",                                  ft_string,                                nil}, 
    {"Data Preparer",                                               ft_string,                                nil}, 
    {"Volume Create Date",                                          ft_string,                                nil}, 
    {"Volume Modify Date",                                          ft_string,                                nil}, 
    {"Boot System",                                                 ft_string,                                nil}, 
    {"Volume Size",                                                 ft_string,                                nil}, 
    {"Artist",                                                      ft_string,                                nil}, 
    {"Album",                                                       ft_string,                                nil}, 
    {"Genre",                                                       ft_string,                                nil}, 
    {"Duration",                                                    ft_string,                                nil}, 
    {"MPEG Audio Version",                                          ft_number,                                nil}, 
    {"Audio Layer",                                                 ft_number,                                nil}, 
    {"Audio Bitrate",                                               ft_string,                                nil}, 
    {"Sample Rate",                                                 ft_number,                                nil}, 
    {"Channel Mode",                                                ft_string,                                nil}, 
    {"MS Stereo",                                                   ft_string,                                nil}, 
    {"Intensity Stereo",                                            ft_string,                                nil}, 
    {"Copyright Flag",                                              ft_string,                                nil}, 
    {"Original Media",                                              ft_number,                                nil}, 
    {"Emphasis",                                                    ft_string,                                nil}, 
    {"ID3 Size",                                                    ft_string,                                nil}, 
    {"Fiction Book Xmlns",                                          ft_string,                                nil}, 
    {"Fiction Book Description Title%-info Author First%-name",     ft_string,                "Author First name"}, 
    {"Fiction Book Description Title%-info Author Middle%-name",    ft_string,               "Author Middle name"}, 
    {"Fiction Book Description Title%-info Author Last%-name",      ft_string,                 "Author Last name"}, 
    {"Fiction Book Description Title%-info Book%-title",            ft_string,                       "Book title"}, 
    {"Fiction Book Description Title%-info Annotation P",           ft_string,                       "Annotation"}, 
    {"Fiction Book Description Title%-info Date",                   ft_string,                             "Date"}, 
    {"Fiction Book Description Title%-info Lang",                   ft_string,                             "Lang"}, 
    {"Fiction Book Description Title%-info Genre",                  ft_string,                            "Genre"}, 
    {"Fiction Book Description Document%-info Author First%-name",  ft_string,  "Document-info Author First name"}, 
    {"Fiction Book Description Document%-info Author Middle%-name", ft_string, "Document-info Author Middle name"}, 
    {"Fiction Book Description Document%-info Author Last%-name",   ft_string,   "Document-info Author Last name"}, 
    {"Fiction Book Description Document%-info Author Nickname",     ft_string,    "Document-info Author Nickname"}, 
    {"Fiction Book Description Document%-info Author Email",        ft_string,       "Document-info Author Email"}, 
    {"Fiction Book Description Document%-info Program%-used",       ft_string,                     "Program used"}, 
    {"Fiction Book Description Document%-info Date",                ft_string,                    "Document Date"}, 
    {"Fiction Book Description Document%-info Id",                  ft_string,                      "Document Id"}, 
    {"Fiction Book Description Document%-info Version",             ft_string,                 "Document Version"}, 
    {"Fiction Book Description Publish%-info Book%-name",           ft_string,           "Publish info Book name"}, 
    {"Fiction Book Description Publish%-info Publisher",            ft_string,                        "Publisher"}, 
    {"Fiction Book Description Publish%-info City",                 ft_string,                             "City"}, 
    {"Fiction Book Description Publish%-info Year",                 ft_number,                             "Year"}, 
    {"Fiction Book Description Publish%-info Isbn",                 ft_string,                             "ISBN"}, 
    {"Fiction Book Body Section Section Annotation P",              ft_string,                       "Annotation"}, 
    {"Description",                                                 ft_string,                                nil}, 
    {"Contributor",                                                 ft_string,                                nil}, 
    {"Language",                                                    ft_string,                                nil}, 
    {"Identifier",                                                  ft_string,                                nil}, 
    {"Compressed Size",                                             ft_number,                                nil}, 
    {"Uncompressed Size",                                           ft_number,                                nil}, 
    {"Zip Compressed Size",                                         ft_number,                                nil}, 
    {"Zip Uncompressed Size",                                       ft_number,                                nil}, 
    {"Operating System",                                            ft_string,                                nil}, 
    {"Packing Method",                                              ft_string,                                nil}, 
    {"Zip Required Version",                                        ft_string,                                nil}, 
    {"Zip Bit Flag",                                                ft_string,                                nil}, 
    {"Zip Compression",                                             ft_string,                                nil}, 
    {"Zip CRC",                                                     ft_string,                                nil}, 
    {"Flags",                                                       ft_string,                                nil}, 
    {"Extra Flags",                                                 ft_string,                                nil}, 
    {"File Attributes",                                             ft_string,                                nil}, 
    {"Target File Size",                                            ft_number,                                nil}, 
    {"Run Window",                                                  ft_string,                                nil}, 
    {"Hot Key",                                                     ft_string,                                nil}, 
    {"Target File DOS Name",                                        ft_string,                                nil}, 
    {"Relative Path",                                               ft_string,                                nil}, 
    {"Local Base Path",                                             ft_string,                                nil}, 
    {"Working Directory",                                           ft_string,                                nil}, 
    {"Command Line Arguments",                                      ft_string,                                nil}, 
    {"Icon File Name",                                              ft_string,                                nil}, 
    {"Page Number",                                                 ft_string,                                nil}, 
}

local filename = ''
local output = ''

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        return getfieldname(FieldIndex + 1), "default|first match", fields[FieldIndex + 1][2];
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return 'EXT="*"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            return nil;
        end
        local handle = io.popen(cmd .. ' "' .. FileName .. '"', 'r');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    if (UnitIndex == 0) then
        return getval(fields[FieldIndex + 1][1], fields[FieldIndex + 1][2]);
    elseif (UnitIndex == 1) then
        if (fields[FieldIndex + 1][3] ~= nil) then
            local hname = getfieldname(FieldIndex + 1);
            local i = 1;
            while (fields[i] ~= nil) do
                if samename(i, hname, fields[FieldIndex + 1][2]) then
                    local value = getval(fields[i][1], fields[i][2]);
                    if (value ~= nil) then
                        return value;
                    end
                end
                i = i + 1;
            end
        else
            return getval(fields[FieldIndex + 1][1], fields[FieldIndex + 1][2]);
        end
    end
    return nil; -- invalid
end

function getval(str, fieldtype)
    if (output ~= nil) then
        local result = output:match("\n" .. str .. "%s*:%s([^\n]+)");
        if (result == nil) then
            return nil;
        end
        if (fieldtype == ft_float) then
            result = result:gsub(",", "%.");
            result = result:gsub("[^%d%.]", '');
            return tonumber(result);
        elseif (fieldtype == ft_number) then
            result = result:match("%d+");
            return tonumber(result);
        elseif (fieldtype == ft_string) then
            return result;
        end
    end
    return nil;
end

function getfieldname(index)
    if (fields[index][3] ~= nil) then
        return fields[index][3];
    else
        return fields[index][1];
    end
    return nil;
end

function samename(index, fieldname, fieldtype)
    if (fields[index][2] == fieldtype) and (getfieldname(index) == fieldname) then
        return true;
    end
    return false;
end
