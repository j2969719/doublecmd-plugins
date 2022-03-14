#!/bin/env lua

-- someshit

args = {...}
os.setlocale("C")
env_var = 'DC_WFX_TP_SCRIPT_DATA'
links_txt = "Select link type:"
symlinks_txt = "Symbolic"
rsymlinks_txt = "Symbolic (relative)"
hardlinks_txt = "Hard"
linkpath_txt = "The link points to"
notalink_tst = "This is not a link."

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

function fs_init()
    print("Fs_MultiChoice " .. links_txt .."\t" .. symlinks_txt .."\t" .. rsymlinks_txt .."\t" .. hardlinks_txt)
    os.exit()
end

function fs_setopt(option, value)
    if option == links_txt then
        if value == rsymlinks_txt then
            print("Fs_Set_" .. env_var .. " rsymlinks")
        elseif value == hardlinks_txt then
            print("Fs_Set_" .. env_var .. " hardlinks")
        else
            print("Fs_Set_" .. env_var .. " symlinks")     
        end
    end
    os.exit()
end

function fs_getlist(path)
    local data = os.getenv(env_var)
    print(data)
    if not path:find('/$') then
        path = path .. '/'
    end
    local ls_output = get_output('ls -lA --time-style="+%Y-%m-%d %R" "' .. path:gsub('"', '\\"') .. '"')
    for line in ls_output:gmatch("[^\n]+") do
        local attr, size, datetime, name = line:match("([%-bcdflsrwxtST]+)%s+%d+%s+[%a%d_]+%s+[%a%d_]+%s+(%d+)%s+(%d%d%d%d%-%d%d%-%d%d%s%d%d:%d%d)%s(.+)") -- https://youtu.be/Fkk9DI-8el4
        if (attr ~= nil and datetime ~= nil and size ~= nil and name ~= nil) then
            name = name:gsub("%s%->%s.+$", "")
            print(attr .. '\t' .. datetime .. '\t' .. size .. '\t' .. name)
        end
    end
    os.exit()
end

function fs_exists(file)
    local f = io.open(file, "r")
    if not f then
        os.exit(1)
    end
    f:close()
    os.exit()
end

function fs_putfile(src, dst)
    local command = nil
    local data = os.getenv(env_var)
    print(data)
    if (data == "rsymlinks") then
        local relpath = get_output('realpath -m --relative-to="' .. dst:match("(.+)/[^/]+$"):gsub('"', '\\"') .. '" "'.. src:gsub('"', '\\"') .. '"')
        command = 'ln -sf "' .. relpath:gsub('"', '\\"') .. '" "' .. dst:gsub('"', '\\"') .. '"'
    elseif (data == "hardlinks") then
        command = 'ln -f "' .. src:gsub('"', '\\"') .. '" "' .. dst:gsub('"', '\\"') .. '"'
    else
        command = 'ln -sf "' .. src:gsub('"', '\\"') .. '" "' .. dst:gsub('"', '\\"') .. '"'
    end
    if (os.execute(command) == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_mkdir(path)
    if (os.execute('mkdir "' .. path:gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end


function fs_properties(file)
    local path = get_output('readlink "' .. file:gsub('"', '\\"') .. '"')
    if (path ~= nil) and (path ~= "") then
       print("Fs_Info_Message ".. linkpath_txt .. ' "' .. path .. '".')
    else
       print("Fs_Info_Message " .. notalink_tst)
    end
    os.exit()
end

function fs_getlocalname(file)
    print(file)
    os.exit()
end

if args[1] == "init" then
    fs_init()
elseif args[1] == "setopt" then
    fs_setopt(args[2], args[3])
elseif args[1] == "list" then
    fs_getlist(args[2])
elseif args[1] == "exists" then
    fs_exists(args[2])
elseif args[1] == "copyin" then
    fs_putfile(args[2], args[3])
elseif args[1] == "mkdir" then
    fs_mkdir(args[2])
elseif args[1] == "properties" then
    fs_properties(args[2])
elseif args[1] == "localname" then
    fs_getlocalname(args[2])
end

os.exit(1)