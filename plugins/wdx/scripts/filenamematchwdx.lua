-- luarocks-5.1 install luautf8

local utf,strfunc = pcall(require,"lua-utf8")
if not utf then
    strfunc = string
end

local group_unknown = "No group"

-- file groups 
local groups = {
    {"Music", -- group name
                    "%.mp3$", -- pattern
                    "%.ogg$",
                    "%.wma$",
                    "%.wav$",
                   "%.flac$",
    },
         
    {"Archive",
                    "%.zip$",
                    "%.rar$",
                     "%.7z$",
                "%.tar%..+$",
    },
    
    {"Docs",
                    "%.doc$",
                   "%.docx$",
                    "%.rtf$",
    },
}

local ft_string = 8
local ft_number = 2
local ft_float  = 3

local pattern = {  
  -- description, field type, pattern,            alt pattern, ...
    {"(number).", ft_number,  "(%d+)%.",          nil,                    nil},
    {nil,         ft_number,  "%-%s+(%d+)%.",     nil,                    nil},
    {"(part) -",  ft_string,  "^(.-)%s+%-",       ".+",                   nil},
    {"- (part)",  ft_string,  "%-%s+%d+%.(.+)$",  ".+%-%s(.+)$",  "%-%s+(.+)"},
    {nil,         ft_string,  "%-%s(.-)%s+%-",    nil,                    nil},
    {"tar.(xyz)", ft_string,  "%.tar%.(.-)$",     nil,                    nil},
}

local UnitsStr = "Filename|Filename.Ext|Ext|Path|Path with Filename.Ext";
local DelimPattern = SysUtils.PathDelim;

if (DelimPattern == nil) then
    DelimPattern = "/\\";
end

function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        local group_names = '';
        for i = 1 , #groups do
            if (i > 1) then
                group_names = group_names .. '|';
            end
            group_names = group_names .. groups[i][1];
        end
        return "Group", group_names, 7; -- FieldName,Units,ft_multiplechoice
    elseif (pattern[FieldIndex] ~= nil) then
            return getFieldName(FieldIndex), UnitsStr, pattern[FieldIndex][2];
    else
        return '', '', 0; -- ft_nomorefields
    end
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (FileName:find("["..DelimPattern.."]%.%.$")) then
        return nil;
    end
    if (FieldIndex == 0) then
        if (#groups >= 1) then
            for group = 1, #groups do
                if (#groups[group] > 1) then
                    for i = 2, #groups[group] do
                        if (strfunc.find(FileName, groups[group][i])) then
                            return groups[group][1];
                        end
                    end
                end
            end
        end
        return group_unknown;
    end
    local isDir = SysUtils.DirectoryExists(FileName);
    local target = getTargetStr(FileName, isDir, UnitIndex);
    if (target ~= nil) and (#pattern[FieldIndex] > 2) then
        for i = 3, #pattern[FieldIndex] do
            local result = strfunc.match(target, pattern[FieldIndex][i]);
            if (result ~= nil) then
                return result;
            end
         end
    end
    return nil;
end

function getFieldName(FieldIndex)
    if (pattern[FieldIndex][1] ~= nil) then
        return pattern[FieldIndex][1];
    elseif (#pattern[FieldIndex] > 2) then
        for i = 3, #pattern[FieldIndex] do
            if (pattern[FieldIndex][i] ~= nil) and (#pattern[FieldIndex] == i) then
                return "Pattern: " .. pattern[FieldIndex][i];
            elseif (pattern[FieldIndex][i] ~= nil) and (#pattern[FieldIndex] > i) then
                return "Pattern: " .. pattern[FieldIndex][i] .. " [...]";
            end
        end
    end
    return nil;
end

function getPath(Str)
    if (Str == nil) then
        return nil;
    end
    return strfunc.match(Str, "(.*[" .. DelimPattern .. "])");
end

function getFullname(Str)
    if (Str == nil) then
        return nil;
    end
    return strfunc.match(Str, "[" .. DelimPattern .. "]([^" .. DelimPattern .. "]+)$");
end

function getFilename(Str, IsDir)
    local FileName = nil;
    local FullName = getFullname(Str);
    if (FullName ~= nil) and (IsDir == false) then
        FileName = strfunc.match(FullName, "(.+)%..+");
    end
    if (FileName ~= nil) then
        return FileName;
    else
        return FullName;
    end
end

function getExt(Str, IsDir)
    if (Str == nil) or (IsDir == true) then
        return nil;
    end
    if (getFilename(Str, IsDir) == getFullname(Str)) then
        return nil;
    end
    return strfunc.match(Str, ".+%.(.+)$");
end

function getTargetStr(FullPath, IsDir, UnitIndex)
    if (UnitIndex == 0) then
        return getFilename(FullPath, IsDir);
    elseif (UnitIndex == 1) then
        return getFullname(FullPath);
    elseif (UnitIndex == 2) then
        return getExt(FullPath, IsDir);
    elseif (UnitIndex == 3) then
        return strfunc.match(FullPath, "(.*[" .. DelimPattern .. "])");
    elseif (UnitIndex == 4) then
        return FullPath;
    end
    return nil;
end
