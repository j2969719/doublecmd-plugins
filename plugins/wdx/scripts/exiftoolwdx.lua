-- exiftoolwdx.lua

local cmd = "exiftool"

local ft_string = 8
local ft_number = 2
local ft_float  = 3
local ft_date   = 10

local fields = {  -- exiftool field name, field type
    {"Machine Type",               ft_string},
    {"Time Stamp",                 ft_string},
    {"PE Type",                    ft_string},
    {"OS Version",                  ft_float},
    {"Subsystem",                  ft_string},
    {"Subsystem Version",          ft_string},
    {"File Version Number",        ft_string},
    {"Product Name",               ft_string},
    {"Product Version Number",     ft_string},
    {"Company Name",               ft_string},
    {"File Description",           ft_string},
    {"Internal Name",              ft_string},
    {"File Version",               ft_string},
    {"Comments",                   ft_string},
    {"Original File Name",         ft_string},
    {"File OS",                    ft_string},
    {"Object File Type",           ft_string},
    {"CPU Architecture",           ft_string},
    {"CPU Type",                   ft_string},
    {"CPU Byte Order",             ft_string},
    {"File Subtype",               ft_string},
    {"File Flags",                 ft_string},
    {"File Flags Mask",            ft_string},
    {"Linker Version",             ft_string},
    {"Code Size",                  ft_string},
    {"Initialized Data Size",      ft_string},
    {"Uninitialized Data Size",    ft_string},
    {"Entry Point",                ft_string},
    {"Language Code",              ft_string},
    {"Character Set",              ft_string},
    {"Legal Copyright",            ft_string},
    {"Legal Trademarks",           ft_string},
    {"Private Build",              ft_string},
    {"Product Version",            ft_string},
    {"Special Build",              ft_string},
    {"Image Version",              ft_string},
    {"Title",                      ft_string},
    {"Subject",                    ft_string},
    {"Author",                     ft_string},
    {"Keywords",                   ft_string},
    {"Last Modified By",           ft_string},
    {"Create Date",                  ft_date},
    {"Modify Date",                  ft_date},
    {"Metadata Date",                ft_date},
    {"Last Printed",                 ft_date},
    {"Revision Number",            ft_string},
    {"Total Edit Time",            ft_string},
    {"Pages",                      ft_number},
    {"Words",                      ft_number},
    {"Characters",                 ft_number},
    {"Lines",                      ft_number},
    {"Paragraphs",                 ft_number},
    {"Characters With Spaces",     ft_number},
    {"Char Count With Spaces",     ft_number},
    {"Internal Version Number",    ft_number},
    {"Company",                    ft_string},
    {"Software",                   ft_string},
    {"App Version",                ft_string},
    {"Code Page",                  ft_string},
    {"Title Of Parts",             ft_string},
    {"Heading Pairs",              ft_string},
    {"Security",                   ft_string},
    {"Scale Crop",                 ft_string},
    {"Links Up To Date",           ft_string},
    {"Shared Doc",                 ft_string},
    {"Tag PID GUID",               ft_string},
    {"Comp Obj User Type",         ft_string},
    {"PDF Version",                ft_string},
    {"Page Count",                 ft_number},
    {"Producer",                   ft_string},
    {"Creator",                    ft_string},
    {"JFIF Version",               ft_string},
    {"GIF Version",                ft_string},
    {"SVG Version",                ft_string},
    {"Resolution Unit",            ft_string},
    {"X Resolution",               ft_number},
    {"Y Resolution",               ft_number},
    {"Image Width",                ft_number},
    {"Image Height",               ft_number},
    {"Color Resolution Depth",     ft_number},
    {"Bit Depth",                  ft_number},
    {"Bits Per Sample",            ft_number},
    {"Color Components",           ft_number},
    {"Frame Count",                ft_number},
    {"Encoding Process",           ft_string},
    {"Y Cb Cr Sub Sampling",       ft_string},
    {"Image Size",                 ft_string},
    {"Megapixels",                 ft_string},
    {"Background Color",           ft_string},
    {"Animation Iterations",       ft_string},
    {"XMP Toolkit",                ft_string},
    {"Creator Tool",               ft_string},
    {"Has Color Map",              ft_string},
    {"Instance ID",                ft_string},
    {"Document ID",                ft_string},
    {"Derived From Instance ID",   ft_string},
    {"Derived From Document ID",   ft_string},
    {"Color Type",                 ft_string},
    {"Compression",                ft_string},
    {"Filter",                     ft_string},
    {"Interlace",                  ft_string},
    {"Pixel Units",                ft_string},
    {"Pixels Per Unit X",          ft_string},
    {"Pixels Per Unit Y",          ft_string},
    {"Publisher",                  ft_string},
    {"Comment",                    ft_string},
    {"Xmlns",                      ft_string},
    {"Volume Name",                ft_string},
    {"Volume Block Count",         ft_number},
    {"Volume Block Size",          ft_number},
    {"Root Directory Create Date",   ft_date},
    {"Data Preparer",              ft_string},
    {"Volume Create Date",           ft_date},
    {"Volume Modify Date",           ft_date},
    {"Boot System",                ft_string},
    {"Volume Size",                ft_string},
    {"Artist",                     ft_string},
    {"Album",                      ft_string},
    {"Track Number",               ft_number},
    {"Genre",                      ft_string},
    {"Duration",                   ft_string},
    {"MPEG Audio Version",         ft_number},
    {"Audio Layer",                ft_number},
    {"Audio Bitrate",              ft_string},
    {"Sample Rate",                ft_number},
    {"Channel Mode",               ft_string},
    {"MS Stereo",                  ft_string},
    {"Intensity Stereo",           ft_string},
    {"Copyright Flag",             ft_string},
    {"Original Media",             ft_number},
    {"Emphasis",                   ft_string},
    {"ID3 Size",                   ft_string},
    {"Opus Version",               ft_string},
    {"Output Gain",                ft_string},
    {"Audio Channels",             ft_string},
    {"Encoder",                    ft_string},
    {"Vendor",                     ft_string},
    {"Description",                ft_string},
    {"Contributor",                ft_string},
    {"Language",                   ft_string},
    {"Identifier",                 ft_string},
    {"Compressed Size",            ft_number},
    {"Uncompressed Size",          ft_number},
    {"Zip Compressed Size",        ft_number},
    {"Zip Uncompressed Size",      ft_number},
    {"Operating System",           ft_string},
    {"Packing Method",             ft_string},
    {"Zip Required Version",       ft_string},
    {"Zip Bit Flag",               ft_string},
    {"Zip Compression",            ft_string},
    {"Zip CRC",                    ft_string},
    {"Flags",                      ft_string},
    {"Extra Flags",                ft_string},
    {"File Attributes",            ft_string},
    {"Target File Size",           ft_number},
    {"Run Window",                 ft_string},
    {"Hot Key",                    ft_string},
    {"Target File DOS Name",       ft_string},
    {"Relative Path",              ft_string},
    {"Local Base Path",            ft_string},
    {"Working Directory",          ft_string},
    {"Command Line Arguments",     ft_string},
    {"Icon File Name",             ft_string},
    {"Page Number",                ft_string},
}

local filename = ''
local output = ''

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        return fields[FieldIndex + 1][1], '', fields[FieldIndex + 1][2];
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
        local handle = io.popen(cmd .. ' "' .. FileName:gsub('"', '\\"') .. '"', 'r');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
    end
    return getval(fields[FieldIndex + 1][1], fields[FieldIndex + 1][2]);
end

function getval(str, fieldtype)
    if (output ~= nil) then
        local result = output:match("\n" .. escapestr(str) .. "%s*:%s([^\n]+)");
        if (result == nil) then
            return nil;
        end
        if (fieldtype == ft_float) then
            result = result:gsub(",", "%.");
            result = result:gsub("[^%d%.]", '');
            if (tonumber(result) == nil) then
                result = result:gsub("%.", ',');
            end
            return tonumber(result);
        elseif (fieldtype == ft_number) then
            result = result:match("%d+");
            return tonumber(result);
        elseif (fieldtype == ft_date) then
            local year, month, day, dhour, dmin, dsec = result:match("^(%d+):(%d+):(%d+)%s(%d+):(%d+):(%d+)");
            if (month ~= nil) and (year ~= nil)  and (day ~= nil) and (dhour ~= nil) and (dmin ~= nil) and (dsec ~= nil) then
                local unixtime = os.time{year = tonumber(year), month = tonumber(month),
                                 day = tonumber(day), hour = tonumber(dhour), min = tonumber(dmin), sec = tonumber(dsec)};
                result = unixtime * 10000000 + 116444736000000000;
                return result;
            else
                return nil;
            end
        elseif (fieldtype == ft_string) then
            return result;
        end
    end
    return nil;
end

function escapestr(str)
    local magic_chars = {"%", ".", "-", "+", "*", "?", "^", "$", "(", ")", "["};
    local result = nil;
    if (str ~= nil) then
        result = str;
        for k, chr in pairs(magic_chars) do
            result = result:gsub("%" .. chr, "%%%" .. chr);
        end
    end
    return result;
end