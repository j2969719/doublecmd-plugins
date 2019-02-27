local DefaultFile = '';
local Replaces = {};
local DefaultDelim = "%s"; -- all space characters, for tab only replace with "	"
local KeepExt = false;
local PathDelim = SysUtils.PathDelim;

local RplFiles = {
  -- filename,             delim, contains extensions
    {"/home/user/test.csv", ",",  false}, 
    {"/home/user/test.txt", "	", true}, 
}

local LuaUtf8, strfunc = pcall(require, "lua-utf8");
if not LuaUtf8 then
    strfunc = string;
end

local convert = nil;
if (LazUtf8 ~= nil) then
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
end

function ContentGetSupportedField(FieldIndex)
    local units = '';
    if (convert ~= nil) then
        units = "utf8|ansi|oem|koi8";
    end
    if (FieldIndex == 0) then
        return GetFullname(DefaultFile), units, 8; -- FieldName,Units,ft_string
    elseif (RplFiles[FieldIndex] ~= nil) then
        return GetFullname(RplFiles[FieldIndex][1]), units, 8; -- FieldName,Units,ft_string
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
                if (UnitIndex == 1) and (line ~= nil) then
                    target = convert(line, "ansi", "utf8");
                elseif (UnitIndex == 2) and (line ~= nil) then
                    target = convert(line, "oem", "utf8");
                elseif (UnitIndex == 3) and (line ~= nil) then
                    target = convert(line, "koi8", "utf8");
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
        return nil;
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