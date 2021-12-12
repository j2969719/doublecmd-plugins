#!/bin/env lua
--[[
⠀⠀⠀⠀⠀⠀⠀⣦⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⣼⣿⣧⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⣀⡻⣿⣿⣆⠀⠀⠀⠀⠀
⠀⠀⠀⠀⣰⣿⣿⣿⣿⣿⣆⠀⠀⠀⠀
⠀⠀⠀⣰⣿⣿⡿⠿⢿⣿⣿⣆⠀⠀⠀
⠀⠀⣰⣿⣿⣿⠀⠀⠈⣿⣿⣿⣆⠀⠀
⠀⣰⣿⣿⠿⠟⠀⠀⠀⠻⠿⣿⣷⣄⠀
⠰⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⠆  btw i use arch®™
]]

args = {...}
temp_file = "/tmp/doublecmd-aur.lst"
update_msg = "Update package list?"

function fs_init()
    os.execute("curl -sS https://aur.archlinux.org/pkgbase.gz | gzip -cd > " .. temp_file)
    os.exit()
end

function fs_setopt(option, value)
    if option == update_msg and value == "Yes" then
        fs_init()
    end
    os.exit()
end

function fs_getlist(path)
    if path == "/" then
        local output = io.popen("stat -c '%Y %s' " .. temp_file)
        if not output then
            os.exit(1)
        end
        local stat_string = output:read()
        output:close()
        if stat_string == nil then
            os.exit(1)
        end
        local stat_mtime, stat_size = stat_string:match("(%d+) (%d+)")
        if stat_mtime == nil or tonumber(stat_size) == 0 then
            os.exit(1)
        end
        local temp_file_date = os.date("!%Y-%m-%dT%TZ", tonumber(stat_mtime))
        for i = 65, 90 do
            print("drwxr-xr-x " .. temp_file_date .. " - " .. string.char(i))
        end
        print("drwxr-xr-x " .. temp_file_date .. " - 0-9")
    else
        local file = io.open(temp_file, "r")
        if not file then
            os.exit(1)
        end
        local current_date = os.date("!%Y-%m-%dT%TZ")
        local pattern = nil
        if path:find("/0%-9") then
            pattern = "^%A"
        else
            local letter = path:match("%a/?$")
            pattern = "^[" .. letter .. letter:lower() .. "]"
        end
        for line in file:lines() do
            if line:find(pattern) and not line:find("# AUR package base list") then
                print("-r--r--r-- " .. current_date .. " 404 " .. line .. ".tar.gz")
            end
        end
        file:close()
    end
    os.exit()
end

function fs_getfile(file, target)
    if file ~= nil and target ~= nil then
        os.execute("curl -sS https://aur.archlinux.org/cgit/aur.git/snapshot" .. file:match("/[^/]+$") .. " > " .. target)
        os.exit()
    end
    os.exit(1)
end

function fs_properties(file)
    if not file:find("%.tar%.gz$") then
        print("Fs_YesNo_Message " .. update_msg)
    else
        os.execute("xdg-open https://aur.archlinux.org/pkgbase" .. file:match("(/[^/]+)%.tar%.gz$"))
    end
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
elseif args[1] == "properties" then
    fs_properties(args[2])
end

os.exit(1)
