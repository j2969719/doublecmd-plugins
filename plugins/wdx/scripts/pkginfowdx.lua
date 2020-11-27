local cmd = "tar -xf"
local params = ".PKGINFO -O"
local output = ''
local filename = ''
local possible_licenses = "GPL|GPL2|GPL3|LGPL|LGPL2.1|LGPL3|AGPL|AGPL3|BSD|MIT|Apache|Python|PHP|RUBY|ZLIB|PSF|ISC|CCPL|CDDL|CPL|FDL|FDL1.2|FDL1.3|LPPL|MPL|MPL2|EPL|ZPL|W3C|Boost|PerlArtistic|Artistic2.0|custom"

local fields = {
    {"pkgname",   8,   "\npkgname = ([^\n]+)"},
    {"pkgver",    8,    "\npkgver = ([^\n]+)"},
    {"pkgdesc",   8,   "\npkgdesc = ([^\n]+)"},
    {"url",       8,       "\nurl = ([^\n]+)"},
    {"packager",  8,  "\npackager = ([^\n]+)"},
    {"license",   7,   "\nlicense = ([^\n]+)"},
    {"license(s)",8,   "\nlicense = ([^\n]+)"},
    {"builddate", 8, "\nbuilddate = ([^\n]+)"},
    {"size",      2,      "\nsize = ([^\n]+)"},
    {"conflict",  8,  "\nconflict = ([^\n]+)"},
    {"provides",  8,  "\nprovides = ([^\n]+)"},
    {"replaces",  8,  "\nreplaces = ([^\n]+)"},
    {"arch",      7,      "\narch = ([^\n]+)"},
    {"raw",       8,                "(.+)\n$"},
}

function ContentGetSupportedField(FieldIndex)
    if (fields[FieldIndex + 1] ~= nil ) then
        if (fields[FieldIndex + 1][1] == "arch") then
            return fields[FieldIndex + 1][1], "x86_64|i686|arm|armv6h|armv6h|aarch64|aarch64", fields[FieldIndex + 1][2];
        elseif (fields[FieldIndex + 1][1] == "license") then
            return fields[FieldIndex + 1][1], possible_licenses, fields[FieldIndex + 1][2];
        else
            return fields[FieldIndex + 1][1], "", fields[FieldIndex + 1][2];
        end
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return '(EXT="XZ")|(EXT="GZ")|(EXT="ZST")';
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FileName:find("%.pkg%.tar%.xz$") == nil) and (FileName:find("%.pkg%.tar%.gz$") == nil) and (FileName:find("%.pkg%.tar%.zst$") == nil) then
        return nil;
    end
    if (filename ~= FileName) then
        local attr = SysUtils.FileGetAttr(FileName);
        if (attr < 0) or (math.floor(attr / 0x00000004) % 2 ~= 0) or (math.floor(attr / 0x00000010) % 2 ~= 0) then
            return nil;
        end
        local handle = io.popen(cmd .. ' "' .. FileName:gsub('"', '\\"') .. '" ' .. params, 'r');
        output = handle:read("*a");
        handle:close();
        filename = FileName;
        if (output == '') or (output == nil) then
            return nil;
        end
    end
    if (output ~= nil) then
        local result = output:match(fields[FieldIndex + 1][3]);
        if (fields[FieldIndex + 1][1] == "builddate") then
            if (result ~= nil) then
                return os.date("%c", tonumber(result));
            end
        else
            return result;
        end
    end
    return nil; -- invalid
end