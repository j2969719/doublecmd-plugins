-- libextractorwdx.lua

local cmd = "extract"

local fields = {
    {"title",                8,                  "\ntitle%s%-%s([^\n]+)"},  -- name, field type, pattern
    {"author name",          8,           "\nauthor%sname%s%-%s([^\n]+)"},
    {"creator",              8,                "\ncreator%s%-%s([^\n]+)"},
    {"format",               8,                 "\nformat%s%-%s([^\n]+)"},
    {"last saved by",        8,        "\nlast%ssaved%sby%s%-%s([^\n]+)"},
    {"created by software",  8,  "\ncreated%sby%ssoftware%s%-%s([^\n]+)"},
    {"produced by software", 8, "\nproduced%sby%ssoftware%s%-%s([^\n]+)"},
    {"encoder version",      8,       "\nencoder%sversion%s%-%s([^\n]+)"},
    {"image dimensions",     8,      "\nimage%sdimensions%s%-%s([^\n]+)"},
    {"comment",              8,                "\ncomment%s%-%s([^\n]+)"},
    {"language",             8,               "\nlanguage%s%-%s([^\n]+)"},
    {"page count",           2,            "\npage%scount%s%-%s([^\n]+)"},
    {"word count",           2,            "\nword%scount%s%-%s([^\n]+)"},
    {"line count",           2,            "\nline%scount%s%-%s([^\n]+)"},
    {"character count",      2,       "\ncharacter%scount%s%-%s([^\n]+)"},
    {"paragraph count",      2,       "\nparagraph%scount%s%-%s([^\n]+)"},
    {"editing cycles",       2,        "\nediting%scycles%s%-%s([^\n]+)"},
    {"creation date",        8,         "\ncreation%sdate%s%-%s([^\n]+)"},
    {"modification date",    8,     "\nmodification%sdate%s%-%s([^\n]+)"},
    {"template",             8,               "\ntemplate%s%-%s([^\n]+)"},
}

local filename = ''
local output = ''

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        return fields[FieldIndex + 1][1], '', fields[FieldIndex + 1][2];
    else
        return '', '', 0; -- ft_nomorefields
    end
end

function ContentGetDefaultSortOrder(FieldIndex)
    return 1; --or -1
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
    if (output ~= nil) and (fields[FieldIndex + 1][3] ~= nil) then
        return output:match(fields[FieldIndex + 1][3]);
    end
    return nil;
end