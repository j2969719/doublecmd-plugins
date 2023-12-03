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
    return output:sub(1, -2)
end

function get_localfile(file)
    return get_output("find " .. os.getenv("HOME") .. "/.cache/yay -maxdepth 2 -type f -name " .. file:sub(2))
end

function fs_init()
    os.exit()
end

function fs_setopt(option, value)
    if option == "Install package" then
        local file = get_localfile(value)
        local cmd = '"sudo pacman -U ' .. file .. '"'
        print("Fs_RunTerm " .. cmd)
        os.exit()
    elseif option == "Make package great again" then
        local dir = get_localfile(value):match("(.*[/])")
        print("Fs_RunTermKeep cd " .. dir .. " && makepkg -srfc")
    elseif option == "Show installed package info" then
        local pkg = os.getenv("DC_WFX_TP_SCRIPT_DATA")
        if not pkg then
            print("Fs_Info_Message DC_WFX_TP_SCRIPT_DATA not set!")
            os.exit(1)
        end
        print("Fs_RunTermKeep pacman -Qiq " .. pkg)
        os.exit()
    end
    os.exit(1)
end

function fs_getlist(path)
    if path == '/' then
        os.execute('find ' .. os.getenv("HOME") .. '/.cache/yay -maxdepth 2 -type f -name "*.pkg.tar.*" -printf "%M %TF %TH:%TM:%TC %s %f\n"')
        os.exit()
    end
    os.exit(1)
end

function fs_properties(file)
    file = get_localfile(file)
    print('path\t'..file)
    local output = get_output("tar -xf " .. file .. " .PKGINFO -O")
    local dsc = output:match("\npkgdesc = ([^\n]+)")
    print("pkgdesc\t" .. dsc)
    local pkg = output:match("\npkgname = ([^\n]+)")
    local icon = get_output("find /usr/share/icons/hicolor/48x48/apps /usr/share/pixmaps -name " .. pkg .. '.png')
    if icon then
        print("png: \t" .. icon)
    end
    print("pkgname\t" .. pkg)
    local ver = output:match("\npkgver = ([^\n]+)")
    print("pkgver\t" .. ver)
    local inst = get_output("LANG= pacman -Qiq " .. pkg .. ' | grep "Version         :"'):sub(19)
    print("installed\t" .. inst)
    local builddate = output:match("\nbuilddate = ([^\n]+)")
    print("builddate\t" .. os.date("%Y-%m-%d %T", tonumber(builddate)))
    local fields = {
        "size",
        "url",
        "packager",
        "license",
        "conflict",
        "provides",
        "replaces",
        "arch",
    }
    for _, field in pairs(fields) do
        value = output:match("\n" .. field .. " = ([^\n]+)")
        if value ~= nil then
            print(field .. '\t' .. value)
        end
    end
    print("Fs_PropsActs Install package\tMake package great again\tShow installed package info")
    print("Fs_Set_DC_WFX_TP_SCRIPT_DATA " .. pkg)
    os.exit()
end

function fs_remove(file)
    file = get_localfile(file)
    if (os.execute('rm "' .. file:gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_getlocalname(file)
    os.execute("find " .. os.getenv("HOME") .. "/.cache/yay -maxdepth 2 -type f -name " .. file:sub(2))
    os.exit()
end

if args[1] == "init" then
    fs_init()
elseif args[1] == "setopt" then
    fs_setopt(args[2], args[3])
elseif args[1] == "list" then
    fs_getlist(args[2])
elseif args[1] == "rm" then
    fs_remove(args[2])
elseif args[1] == "properties" then
    fs_properties(args[2])
elseif args[1] == "localname" then
    fs_getlocalname(args[2])
end

os.exit(1)
