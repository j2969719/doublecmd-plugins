#!/bin/env lua

-- someshit

adb_cmd = "adb" --touch


args = {...}

function get_output(command)
    local handle = io.popen(command, 'r')
    if not handle then
        os.exit(1)
    end
    local output = handle:read("*a")
    handle:close()
    if output == nil then
        os.exit(1)
    end
    return output
end

function fs_init()
    os.execute(adb_cmd .. " start-server")
    os.exit()
end

function fs_getlist(path)
    if not path:find('/$') then
        path = path .. '/'
    end
    local ls_output = get_output(adb_cmd .. ' shell ls -lA "' .. path:gsub(" ", "\\ "):gsub('"', '\\"') .. '"')
    for line in ls_output:gmatch("[^\n]+") do
        local attr, size, datetime, name = line:match("([%-bcdflsrwx]+)%s+%d+%s+[%a%d_]+%s+[%a%d_]+%s+(%d+)%s+(%d%d%d%d%-%d%d%-%d%d%s%d%d:%d%d)%s(.+)") -- https://youtu.be/Fkk9DI-8el4
        if (attr ~= nil and datetime ~= nil and size ~= nil and name ~= nil) then
            name = name:gsub("%s%->%s.+$", "")
            print(attr .. '\t' .. datetime .. '\t' .. size .. '\t' .. name)
        end
    end
    os.exit()
end

function fs_getfile(src, dst)
    if (os.execute(adb_cmd .. ' pull "' .. src:gsub('"', '\\"') .. '" "' .. dst:gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_exists(file)
    if (os.execute(adb_cmd .. ' shell [[ -e "' .. file:gsub(" ", "\\ "):gsub('"', '\\"') .. '" ]]') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_putfile(src, dst)
    if (os.execute(adb_cmd .. ' push "' .. src:gsub('"', '\\"') .. '" "' .. dst:gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_copy(src, dst)
    if (os.execute(adb_cmd .. ' shell cp "' .. src:gsub(" ", "\\ "):gsub('"', '\\"') .. '" "' .. dst:gsub(" ", "\\ "):gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_move(src, dst)
    if (os.execute(adb_cmd .. ' shell mv "' .. src:gsub(" ", "\\ "):gsub('"', '\\"') .. '" "' .. dst:gsub(" ", "\\ "):gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_remove(path)
    if (os.execute(adb_cmd .. ' shell rm -rf "' .. path:gsub(" ", "\\ "):gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_mkdir(path)
    if (os.execute(adb_cmd .. ' shell mkdir "' .. path:gsub(" ", "\\ "):gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_execute(file)
    local output = get_output(adb_cmd .. ' shell readlink "' .. file:gsub(" ", "\\ "):gsub('"', '\\"') .. '"')
    if (output ~= nil) then
        output = output:sub(1, -2)
        if (os.execute(adb_cmd .. ' shell [[ -d "' .. output:gsub(" ", "\\ "):gsub('"', '\\"') .. '" ]]') == true) then
            print(debug.getinfo(1, 'S').source:match('(/.-)$') .. output)
            os.exit()
        end
    end
    os.exit(1)
end

function fs_properties(file)
    local props = {
        {"Size",                "Size:%s(%d+)"},
        {"Uid",             "Uid:%s%(([^%)]+)"},
        {"Gid",             "Uid:%s%(([^%)]+)"},
        {"Access", "Access:%s([%d%s%-%.:]+)\n"},
        {"Modify", "Modify:%s([%d%s%-%.:]+)\n"},
        {"Change", "Change:%s([%d%s%-%.:]+)\n"},
    }
    if (os.execute(adb_cmd .. ' shell [[ -d "' .. file:gsub(" ", "\\ "):gsub('"', '\\"') .. '" ]]') == true) then
        print('content_type\tinode/directory')
    end
    local output = get_output(adb_cmd .. " shell stat '" .. file:gsub(" ", "\\ "):gsub("'", "\\'") .. "'")
    if (output ~= nil) then
        for _, field in pairs(props) do
            value = output:match(field[2])
            if (value ~= nil) then
                print(field[1] .. '\t' .. value)
            end
        end
    end
    os.exit()
end


function fs_quote(str, path)
    if (os.execute(adb_cmd .. " shell '" .. str:gsub("'", "\\'") .. "' 1>&2") == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_deinit()
    os.execute(adb_cmd .. ' kill-server')
    os.exit()
end


if args[1] == "init" then
    fs_init()
elseif args[1] == "list" then
    fs_getlist(args[2])
elseif args[1] == "copyout" then
    fs_getfile(args[2], args[3])
elseif args[1] == "exists" then
    fs_exists(args[2])
elseif args[1] == "copyin" then
    fs_putfile(args[2], args[3])
elseif args[1] == "cp" then
    fs_copy(args[2], args[3])
elseif args[1] == "mv" then
    fs_move(args[2], args[3])
elseif args[1] == "mkdir" then
    fs_mkdir(args[2])
elseif args[1] == "rm" then
    fs_remove(args[2])
elseif args[1] == "rmdir" then
    fs_remove(args[2])
elseif args[1] == "run" then
    fs_execute(args[2])
elseif args[1] == "properties" then
    fs_properties(args[2])
elseif args[1] == "quote" then
    fs_quote(args[2], args[3])
elseif args[1] == "deinit" then
    fs_deinit()
end

os.exit(1)
