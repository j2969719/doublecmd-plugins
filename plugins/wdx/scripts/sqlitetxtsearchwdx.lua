-- ...

local sqlite3 = require("lsqlite3")

local fields = {
    {"EvictionInfoTable", "origin"}, -- table, column
    {"Cars",                "Name"},
    {"Cars",               "Price"},
}
function ContentGetSupportedField(FieldIndex)
    if (FieldIndex == 0) then
        return 'Tables', '', 9;
    elseif (fields[FieldIndex] ~= nil) then
        return fields[FieldIndex][1] .. '.' .. fields[FieldIndex][2], '', 9;
    end
    return '', '', 0; -- ft_nomorefields
end

function ContentGetDetectString()
    return 'EXT="DB" | EXT="DB3" | EXT="SQLITE"'; -- return detect string
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if (SysUtils.DirectoryExists(FileName)) then
        return nil;
    end
    if (UnitIndex == 0) then
        local query;
        local tmp = {};
        local db = sqlite3.open(FileName, sqlite3.OPEN_READ);
        if (db == nil) then
            return nil;
        end
        if (FieldIndex == 0) then
            query = "SELECT tbl_name FROM sqlite_master WHERE type = 'table'";
        elseif (fields[FieldIndex][1] ~= nil) and (fields[FieldIndex][2] ~= nil) then
            query = "SELECT " .. fields[FieldIndex][2] .. " FROM " .. fields[FieldIndex][1];
        end
        if  (db:exec(query) == sqlite3.OK) then
            for row in db:urows(query) do
                table.insert(tmp, row .. '\n');
            end
        end
        db:close();
        local result = table.concat(tmp);
        return result:sub(1, - 2);
    end
    return nil; -- invalid
end
