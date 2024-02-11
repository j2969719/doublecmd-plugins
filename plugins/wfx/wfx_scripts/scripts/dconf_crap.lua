#!/bin/env lua

-- someshit

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

function fs_getlist(path)
    if not path:find('/$') then
        path = path .. '/'
    end
    local output = get_output("dconf list " .. path)
    local dirs = '\t'
    for line in output:gmatch("[^\n]+") do
        if line:find('/$') then
            print("drwxr-xr-x\t0000-00-00 00:00:00\t-\t" .. line:sub(1, -2))
            dirs = dirs .. path .. line:sub(1, -2) .. '\t'
        else
            print("-rw-rw-rw-\t0000-00-00 00:00:00\t-\t" .. line)
        end
    end
    print("Fs_Set_DC_WFX_SCRIPT_DIRS " .. dirs)
    os.exit()
end

function fs_getfile(src, dst)
    if (os.execute("dconf read " .. src .. '> "' .. dst:gsub('"', '\\"') .. '"') == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_exists(file)
    if (os.execute("dconf read " .. file) == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_putfile(src, dst)
    for line in io.lines(src) do
        if (os.execute("dconf write " .. dst .. ' "' .. line .. '"') == true) then
            os.exit()
        end
        io.stderr:write("dconf write " .. dst .. ' ' .. line)
        break
    end
    os.exit(1)
end

function fs_rm(file)
    if (os.execute("dconf reset " .. file) == true) then
        os.exit()
    end
    os.exit()
end

function fs_rmdir(path)
    if (os.execute("dconf reset " .. path) == true) then
        os.exit()
    end
    os.exit()
end

function fs_properties(file)
    local dirs = os.getenv("DC_WFX_SCRIPT_DIRS")
    if dirs:find('\t' .. file .. '\t') then
        file = file .. '/'
    end
    print("Fs_RunTermKeep dconf watch " .. file)
    os.exit()
end


if args[1] == "list" then
    fs_getlist(args[2])
elseif args[1] == "copyout" then
    fs_getfile(args[2], args[3])
elseif args[1] == "exists" then
    fs_exists(args[2])
elseif args[1] == "copyin" then
    fs_putfile(args[2], args[3])
elseif args[1] == "rm" then
    fs_rm(args[2])
elseif args[1] == "rmdir" then
    fs_rmdir(args[2])
elseif args[1] == "properties" then
    fs_properties(args[2])
end

os.exit(1)
