#!/bin/env lua

-- all stars nico nico douga insanity

args = {...}
env_var = 'DC_WFX_SCRIPT_DATA'
msg_repo = "Path to git repo"
msg_treeish = "Branch/Tag"
msg_custom = "<Custom tree-ish>"

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

function get_data()
    local data = os.getenv(env_var)
    if not data then
        os.exit(1)
    end
    local dir = data:match("[^\t]+")
    local treeish = data:match("\t(.+)")
    if not treeish or not dir then
        os.exit(1)
    end
    return dir:gsub('"', '\\"'), treeish
end

function fs_init()
    print('Fs_Request_Options')
    print(msg_repo)
    os.exit()
end

function fs_setopt(option, value)
    if option == msg_repo then
        if value == '' then
            io.stderr:write("Path to git repo is empty")
            os.exit(1)
        end
        print("Fs_Set_DC_WFX_SCRIPT_DATA " .. value)
        local output = get_output('cd "' .. value .. '" && git branch')
        local treeish = ''
        for line in output:gmatch("[^\n]-\n") do
            treeish = treeish .. '\t' .. line:match("%s?%*?%s?(.+)\n")
        end
        local output = get_output('cd "' .. value .. '" && git tag')
        for line in output:gmatch("[^\n]-\n") do
            treeish = treeish .. '\t' .. line:match("(.+)\n")
        end
        if treeish ~= '' then
            print("Fs_MultiChoice " .. msg_treeish .. treeish .. '\t' .. msg_custom)
        end
    elseif option == msg_treeish or option == msg_custom then
        if value == '' then
            io.stderr:write("Treeish is empty")
            os.exit(1)
        end
        if value == msg_custom then
            print('Fs_Request_Options')
            print(msg_custom)
            os.exit()
        end
        local data = os.getenv(env_var)
        print("Fs_Set_DC_WFX_SCRIPT_DATA " .. data .. '\t' .. value)
    end
    os.exit()
end

function fs_getlist(path)
    local dir, treeish = get_data()
    local dirpath = ""
    if path ~= '/' then
        dirpath = path:gsub('^/', ' "') .. '/"'
    end
    local output = get_output('cd "' .. dir .. '" && git ls-tree -l ' ..  treeish .. dirpath)
    for line in output:gmatch("[^\n]-\n") do
        local mode, objtype, objname, filesize, pathname = line:match('(%d+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+');
        print(mode .. " 0000-00-00 00:00:00 " .. filesize .. " " .. pathname:match("([^/]+)$"))
    end
    os.exit()
end

function fs_getfile(file, target)
    local dir, treeish = get_data()
    os.execute('cd "' .. dir .. '" && git show "' ..  treeish .. file:gsub('^/', ':'):gsub('"', '\\"')  .. '" > "' .. target:gsub('"', '\\"') .. '"')
    os.exit()
end

function fs_properties(file)
    local dir, treeish = get_data()
    local output = get_output('cd "' .. dir .. '" && git ls-tree -l ' ..  treeish .. ' "' .. file:gsub('^/', ''):gsub('"', '\\"') .. '"')
    local mode, objtype, objname, filesize, pathname = output:match('(%d+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+');
    print('Treeish\t'..treeish)
    print('Object\t'..objtype)
    print('Name\t'..objname)
    print('Mode\t'..mode)
    print('Size\t'..filesize)
    print('Path\t'..pathname)
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
