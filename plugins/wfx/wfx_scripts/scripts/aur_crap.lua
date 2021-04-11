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

function fs_init()
    os.execute("curl https://aur.archlinux.org/pkgbase.gz | gzip -cd > " .. temp_file)
    print("Disable_FsStatusInfo")
    os.exit()
end

function fs_getlist(path)
    if path == "/" then
        local f = io.popen("stat -c %Y ".. temp_file)
        local stat_output = f:read()
        f:close()
        if stat_output == nil then
            os.exit(1)
        end
        local temp_file_date = os.date("!%Y-%m-%dT%TZ", stat_output)
        for i = 65, 90 do
            print("drwxr-xr-x " .. temp_file_date .. " - " .. string.char(i))
        end
        print("drwxr-xr-x " .. temp_file_date .. " - _other")
    else
        local file = io.open(temp_file, "r")
        if not file then
            os.exit(1)
        end
        local current_date = os.date("!%Y-%m-%dT%TZ")
        local pattern = nil
        if path:find("_other") then
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
        os.execute("curl https://aur.archlinux.org/cgit/aur.git/snapshot" .. file:match("/[^/]+$") .. " > " .. target)
        os.exit()
    end
    os.exit(1)
end

function fs_properties(file)
    if not file:find("%.tar%.gz$") then
        fs_init()
    end
    os.execute("xdg-open https://aur.archlinux.org/pkgbase" .. file:match("(/[^/]+)%.tar%.gz$"))
    os.exit()
end

if args[1] == "init" then
    fs_init()
elseif args[1] == "list" then
    fs_getlist(args[2])
elseif args[1] == "copyout" then
    fs_getfile(args[2], args[3])
elseif args[1] == "properties" then
    fs_properties(args[2])
end

os.exit(1)
