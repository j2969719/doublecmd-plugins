#!/bin/env lua

-- all stars nico nico douga insanity

args = {...}
env_var = 'DC_WFX_SCRIPT_DATA'
msg_repo = "Path to bare git repo or .git"

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
    local dir = data:match("[^\t]+")
    local branch = data:match("\t(.+)")
    return dir, branch
end

function fs_init()
    print('Fs_Request_Options')
    print(msg_repo)
    os.exit()
end

function fs_setopt(option, value)
    if option == msg_repo then
        print("Fs_Set_DC_WFX_SCRIPT_DATA " .. value)
        local output = get_output('git --git-dir="' .. value .. '" branch')
        local branches = ''
        for line in output:gmatch("[^\n]-\n") do
            branches = branches .. '\t' .. line:match("%s?%*?%s?(.+)\n")
        end
        print("Fs_MultiChoice Branch" .. branches)
    elseif option == "Branch" then
        local data = os.getenv(env_var)
        print("Fs_Set_DC_WFX_SCRIPT_DATA " .. data .. '\t' .. value)
    end
    os.exit()
end

function fs_getlist(path)
    local dir, branch = get_data()
    local dirpath = ""
    if path ~= '/' then
        dirpath = path:gsub('^/', ' ') .. '/'
    end
    local output = get_output('git --git-dir="' .. dir .. '" ls-tree -l ' ..  branch .. dirpath)
    for line in output:gmatch("[^\n]-\n") do
        local mode, objtype, objname, filesize, pathname = line:match('(%d+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+');
        print(mode .. " 0000-00-00 00:00:00 " .. filesize .. " " .. pathname:match("([^/]+)$"))
    end
    os.exit()
end

function fs_getfile(file, target)
    local dir, branch = get_data()
    os.execute('git --git-dir="' .. dir .. '" show ' ..  branch .. file:gsub('^/', ':')  .. ' > ' .. target)
    os.exit()
end

function fs_properties(file)
    local dir, branch = get_data()
    local output = get_output('git --git-dir="' .. dir .. '" ls-tree -l ' ..  branch .. file:gsub('^/', ' '))
    local mode, objtype, objname, filesize, pathname = output:match('(%d+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+');
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
