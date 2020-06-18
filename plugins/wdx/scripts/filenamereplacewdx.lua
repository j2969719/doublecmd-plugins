local DefaultFile = '';
local Replaces = {};
local DefaultDelim = "%s"; -- all space characters, for tab only replace with "\t"
local KeepExt = false;
local PathDelim = SysUtils.PathDelim;

local RplFiles = {
  -- file path,             delim, contains extensions
    {"/home/user/test.csv",   ",",  false},
    {"home/user/test.txt",    "\t",  true},
    {"C:\\SomeDir\\winf.txt", "\t", false},
}

local LuaUtf8, strfunc = pcall(require, "lua-utf8");
if not LuaUtf8 then
    strfunc = string;
end

local convert = nil;
local units = '';
local encoding = {
--[[
List of supported encoding values:

 Default system encoding (depends on user locale): "default",
 Default ANSI (Windows) encoding (depends on user locale): "ansi",
 Default OEM (DOS) encoding (depends on user locale): "oem",
 ANSI (Windows): "cp1250", "cp1251", "cp1252", "cp1253", "cp1254", "cp1255", "cp1256", "cp1257", "cp1258",
 OEM (DOS): "cp437", "cp850", "cp852", "cp866", "cp874", "cp932", "cp936", "cp949", "cp950",
 ISO 8859: "iso88591", "iso88592", "iso885915",
 Other: "macintosh", "koi8",
http://doublecmd.github.io/doc/en/lua.html#libraryutf8
]]
    "ansi",
    "oem",
    "koi8",
}
if LazUtf8 then
    convert = LazUtf8.ConvertEncoding;
end

function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
    if (PathDelim == nil) then
        if (IniFileName:match('.') == '/') then
            PathDelim = '/';
        else
            PathDelim = '\\';
        end
    end
    DefaultFile = GetPath(IniFileName) .. "replaces.txt";
    local file = io.open(DefaultFile, 'r');
    if (file == nil) then
        file = io.open(DefaultFile, 'w');
    end
    if (file ~= nil) then
        file:close();
    end
    if (convert ~= nil) then
        units = "utf8";
        for i = 1 , #encoding do
            units = units .. '|' .. encoding[i];
        end
    end
end

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return strfunc.match(DefaultFile, "([^/\\]+)$"), units, 8; -- FieldName,Units,ft_string
    elseif (RplFiles[FieldIndex] ~= nil) then
        return strfunc.match(RplFiles[FieldIndex][1], "([^/\\]+)$"), units, 8; -- FieldName,Units,ft_string
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    local keepext = false;
    if (FieldIndex == 0) then
        UpdateReplaces(DefaultFile, DefaultDelim, UnitIndex);
        keepext = KeepExt;
    else
        local delim = RplFiles[FieldIndex][2];
        if (delim == nil) then
            delim = DefaultDelim;
        end
        UpdateReplaces(RplFiles[FieldIndex][1], delim, UnitIndex);
        keepext = RplFiles[FieldIndex][3];
        if (keepext == nil) then
            keepext = KeepExt;
        end
    end
    local target = nil;
    if not keepext then
        target = GetFilename(FileName, SysUtils.DirectoryExists(FileName));
    else
        target = GetFullname(FileName);
    end
    if (target == nil) then
        return nil;
    end
    local index = LowerCase(target);
    if (Replaces[index] ~= nil) then
        return Replaces[index];
    else
        return target;
    end
end

function UpdateReplaces(FileName, Delim, UnitIndex)
    Replaces = {};
    local delim = Delim;
    if (delim == nil) then
        delim = DefaultDelim;
    end
    if (FileName ~= nil) then
        local file = io.open(FileName, 'r');
        if (file ~= nil) then
            for line in file:lines() do
                local target = line;
                if (line ~= nil) and (convert ~= nil) then
                    if (UnitIndex > 0) and (encoding[UnitIndex] ~= nil) then
                        target = convert(line, encoding[UnitIndex], "utf8");
                    end
                end
                if (target ~= nil) then
                    if (strfunc.match(target, '^.') == '"') then
                        org  = strfunc.match(target, '^"([^"]+)');
                    else
                        org  = strfunc.match(target, '^([^' .. delim .. ']+)');
                    end
                    if (strfunc.match(target, '.$') == '"') then
                        repl = strfunc.match(target, '([^"]+)"$');
                    else
                        repl = strfunc.match(target, '([^' .. delim .. ']+)$');
                    end
                    if (org ~= nil) and (repl ~= nil) then
                        org  = LowerCase(org);
                        Replaces[org] = repl;
                    end
                end
            end
            file:close();
        end
    end
end

function GetPath(String)
    if (String == nil) then
        return '';
    end
    return strfunc.match(String, ".*" .. PathDelim);
end

function GetFullname(String)
    if (String == nil) then
        return nil;
    end
    return strfunc.match(String, "([^" .. PathDelim .. "]+)$");
end

function GetFilename(String, IsDir)
    local result = nil;
    local fullname = GetFullname(String);
    if (fullname ~= nil) and not IsDir then
        result = strfunc.match(fullname, "(.+)%..+");
    end
    if (result ~= nil) then
        return result;
    else
        return fullname;
    end
end

function LowerCase(String)
    local result = '';
    if not LuaUtf8 and LazUtf8 then
        result = LazUtf8.LowerCase(String);
    else
        result = strfunc.lower(String);
    end
    return result;
end