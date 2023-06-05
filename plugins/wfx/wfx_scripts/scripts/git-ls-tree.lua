#!/bin/env lua

-- show commit date in filelist (slow af)
get_dates = false
date_form = "%ad" -- %ad - author date, %cd - commiter date

args = {...}
env_var = "DC_WFX_SCRIPT_DATA"
msg_repo = "Path to git repo"
msg_treeish = "Tree-ish"
msg_custom = "<Custom tree-ish>"
msg_commit = "<Commit>"
act_select = "Select tree-ish"
act_commit = "Select commit"
act_changes = "Show changes"


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
    return dir, treeish
end

function select_treeish(dir)
    local treeish = '\t' .. msg_commit .. '\t' .. msg_custom
    local output = get_output('cd ' .. dir .. ' && git branch')
    for line in output:gmatch("[^\n]-\n") do
        treeish = treeish .. '\t' .. line:match("%s?%*?%s?(.+)\n")
    end
    local output = get_output('cd ' .. dir .. ' && git tag')
    for line in output:gmatch("[^\n]-\n") do
        treeish = treeish .. '\t' .. line:match("(.+)\n")
    end
    print("Fs_MultiChoice " .. msg_treeish .. treeish)
end

function where_am_i(dir, treeish)
    print("Fs_LogInfo")
    os.execute('cd ' .. dir .. ' && git describe --always ' ..  treeish .. ' --')
    os.execute('cd ' .. dir .. ' && git log -1 --pretty=format:"%an (%ae)\n%ad\n%s\n%b" --date=format:"%Y-%m-%d %T" ' ..  treeish .. ' --')
end

function show_log(dir, treeish)
    local output = get_output('cd ' .. dir .. ' && git log --oneline ' ..  treeish)
    local commits = ''
    for line in output:gmatch("[^\n]-\n") do
        commits = commits .. '\t' .. line:match("(.+)\n")
    end
    print("Fs_MultiChoice " .. msg_commit .. '\t' .. commits)
    os.exit()
end

function show_changes(dir, treeish)
    local output = get_output('cd ' .. dir .. ' && git log -1 --pretty=format:%h ' ..  treeish .. ' --')
    print("Fs_LogInfo")
    os.execute('cd ' .. dir .. ' && git show ' ..  output .. ' --')
end

function fs_init()
    -- DC 1.0.10 or older
    -- print("Fs_Request_Options")
    -- print(msg_repo)

    -- DC 1.0.11+ only
    print("Fs_SelectDir " .. msg_repo)
    os.exit()
end

function fs_setopt(option, value)
    if option == msg_repo then
        if value == '' then
            io.stderr:write("Path to git repo is empty")
            os.exit(1)
        end
        local dir = '"' .. value:gsub('"', '\\"') .. '"'
        print("Fs_Set_" .. env_var .. ' ' .. dir)
        select_treeish(dir)
    elseif option == msg_treeish or option == msg_custom then
        if value == '' then
            io.stderr:write("Tree-ish is empty")
            os.exit(1)
        end
        if value == msg_custom then
            print("Fs_Request_Options")
            print(msg_custom)
            os.exit()
        end
        local data = os.getenv(env_var):match("[^\t]+")
        if value == msg_commit then
            show_log(data, "HEAD")
            os.exit()
        end
        print("Fs_Set_" .. env_var .. ' ' .. data .. '\t' .. value)
        where_am_i(data, value)
    elseif option == msg_commit then
        local data = os.getenv(env_var)
        local dir = data:match("[^\t]+")
        local commit = value:match("(.-)%s")
        print("Fs_Set_" .. env_var .. ' ' .. dir .. '\t' .. commit)
        where_am_i(dir, commit)
    else
        local dir, treeish = get_data()
        if option == act_changes then
            show_changes(dir, treeish)
        elseif option == act_select then
            select_treeish(dir)
        elseif option == act_commit then
            show_log(dir, treeish)
        end
    end
    os.exit()
end

function fs_getlist(path)
    local dir, treeish = get_data()
    local dirpath = ""
    if path ~= '/' then
        dirpath = path:gsub('"', '\\"'):gsub('^/', ' "') .. '/"'
    end
    local output = get_output('cd ' .. dir .. ' && git ls-tree -l ' ..  treeish .. dirpath)
    for line in output:gmatch("[^\n]-\n") do
        local mode, objtype, objname, filesize, pathname = line:match('(%d+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+')
        local datetime = nil
        if get_dates then
            datetime = get_output('cd ' .. dir .. ' && git log -1 --pretty=format:' .. date_form .. ' --date=format:"%Y-%m-%d %T" ' ..  treeish .. ' -- "' .. pathname:gsub('"', '\\"') .. '"')
        end
        if not datetime then
            datetime = "0000-00-00 00:00:00"
        end
        print(mode .. ' ' .. datetime .. ' ' .. filesize .. ' ' .. pathname:match("([^/]+)$"))
    end
    os.exit()
end

function fs_getfile(file, target)
    local dir, treeish = get_data()
    os.execute('cd ' .. dir .. ' && git show "' ..  treeish .. file:gsub('^/', ':'):gsub('"', '\\"')  .. '" > "' .. target:gsub('"', '\\"') .. '"')
    os.exit()
end

function fs_properties(file)
    local dir, treeish = get_data()
    local output = get_output('cd ' .. dir .. ' && git ls-tree -l ' ..  treeish .. file:gsub('"', '\\"'):gsub('^/', ' -- "') .. '"')
    local mode, objtype, objname, filesize, pathname = output:match('(%d+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+');
    -- print('Object\t'..objtype)
    if objtype == "tree" then
        print("content_type\tinode/directory")
    end
    -- print('Name\t'..objname)
    output = get_output('cd ' .. dir .. ' && git log -1 --pretty=format:"%ad\n%an (%ae)\n%s" --date=format:"%Y-%m-%d %T" ' ..  treeish .. file:gsub('"', '\\"'):gsub('^/', ' -- "') .. '"')
    local datetime, author, subject = output:match("([^\n]-)\n([^\n]-)\n(.+)$")
    print("Author\t" .. author)
    print("Subject\t" .. subject)
    print("Author date\t" .. datetime)
    print("Size\t" .. filesize)
    print("Mode\t" .. mode)
    print("Path\t" .. pathname)
    print("Tree-ish\t" .. treeish)
    output = get_output('cd ' .. dir .. ' && git log -1 --pretty=format:"%cd\n%cn (%ce)\n%s" --date=format:"%Y-%m-%d %T" ' ..  treeish)
    local datetime, commiter, subject = output:match("([^\n]-)\n([^\n]-)\n(.+)$")
    print("Commiter\t" .. commiter)
    print("Subject\t" .. subject)
    print("Commiter date\t" .. datetime)
    print("Fs_PropsActs " .. act_changes .. '\t' .. act_commit .. '\t' .. act_select)
    os.exit()
end

function fs_quote(str, path)
    local dir, treeish = get_data()
    if str == "commit" then
        show_log(dir, treeish)
    elseif str == "select" then
        select_treeish(dir)
    elseif str == "where" then
        where_am_i(dir, treeish)
    elseif str == "changes" then
        show_changes(dir, treeish)
    else
        os.exit(1)
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
elseif args[1] == "quote" then
    fs_quote(args[2], args[3])
end

os.exit(1)
