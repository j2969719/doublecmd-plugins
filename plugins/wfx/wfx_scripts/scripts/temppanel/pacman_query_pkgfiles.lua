#!/bin/env lua

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

function get_pkgname(path)
    return path:match("^/(.-)/"):match("(.+)%s[^%s]-$")
end

function get_realname(path)
    return path:match("^/.-(/.*)")
end

function fs_init()
    os.exit()
end

function fs_setopt(option, value)
    os.exit()
end

function fs_getlist(path)
    if path == "/" then
        local output = get_output("pacman -Q")
        for pkg in output:gmatch("[^\n]-\n") do
            print("dr-xr-xr-x 0000-00-00 00:00:00 - " .. pkg)
        end
    else
        local files = {}
        if not path:find("/$") then
           path = path .. '/'
        end
        local pkgname = get_pkgname(path)
        local target = path:match("^/.-(/.*)")
        local output = get_output("pacman -Qlq " .. pkgname)
        for line in output:gmatch("[^\n]+") do
            if line:sub(1, target:len()) == target then
                file = line:sub(target:len() + 1, -1):match("^(.-)/")
                if not file then
                    file = line:sub(target:len() + 1, -1)
                end
                if file ~= '' and files[file] ~= true then
                    local stat_string = get_output("stat -c '%Y %s %A' " .. target .. file)
                    local mtime, size, attr = stat_string:match("(%d+) (%d+) ([^\n]+)")
                    if attr:sub(1, 1) == 'd' then
                        size = '-'
                    end
                    print(attr .. '\t' .. os.date("!%Y-%m-%dT%TZ", tonumber(stat_mtime)) .. '\t' .. size .. '\t' .. file)
                end
                files[file] = true
            end
        end
    end
    os.exit()
end

function fs_getfile(file, target)
    local realname = get_realname(file)
    if realname ~= nil then
        os.execute('cp -P "' .. realname:gsub('"', '\\"') .. '" "' .. target:gsub('"', '\\"') .. '"')
    end
    os.exit()
end

function fs_properties(file)
    local realname = get_realname(file)
    if realname ~= nil then
        os.execute('dbus-send  --dest=org.freedesktop.FileManager1 --type=method_call /org/freedesktop/FileManager1 org.freedesktop.FileManager1.ShowItemProperties array:string:"file://' .. realname:gsub('"', '\\"') .. '", string:"0"')
    else
        print('content_type\tinode/directory')
        local fields = {
            "Description",
            "Architecture",
            "URL",
            "Licenses",
            "Depends On",
            "Required By",
            "Optional Deps",
            "Installed Size",
            "Build Date",
            "Install Date",
            "Install Reason",
        }
        local pkgname = get_pkgname(file .. '/')
        local output = get_output("LANG= pacman -Qiq " .. pkgname)
        for _, field in pairs(fields) do
            value = output:match(field .. '%s+:%s+([^\n]-)\n')
            if value ~= nil then
                print(field..'\t'..value)
            end
        end
    end
    os.exit()
end

function fs_execute(file)
    os.execute('xdg-open "' .. get_realname(file):gsub('"', '\\"') .. '" &')
    os.exit()
end

function fs_getlocalname(file)
    print(get_realname(file))
    os.exit()
end

if args[1] == "init" then
    fs_init()
elseif args[1] == "setopt" then
    fs_setopt(args[2], args[3])
elseif args[1] == "list" then
    fs_getlist(args[2])
elseif args[1] == "copyout" then
    fs_getfile(args[2], args[3])
elseif args[1] == "run" then
    fs_execute(args[2])
elseif args[1] == "properties" then
    fs_properties(args[2])
elseif args[1] == "localname" then
    fs_getlocalname(args[2])
end

os.exit(1)
