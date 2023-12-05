#!/bin/env lua

args = {...}


function log_err(str)
    if not str then
        return
    end
    if not str:find("\n$") then
        str = str .. '\n'
    end
    io.stderr:write(str)
end

function get_text(tepmlate)
    text = os.getenv(tepmlate)
    if not text then
        text = tepmlate
    end
    return text
end

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
    local dir = os.getenv("DC_WFX_SCRIPT_DIR")
    local treeish = os.getenv("DC_WFX_SCRIPT_TREEISH")
    if not treeish or not dir then
        os.exit(1)
    end
    return dir, treeish
end

function print_prop(name, value)
    if value ~= nil then
        print(name .. '\t' .. value)
    end
end

function select_treeish(dir)
    local extra = "\tWFX_SCRIPT_STR_COMMIT\tWFX_SCRIPT_STR_CUSTOM\tWFX_SCRIPT_STR_REFLOG"
    local treeish = extra
    local output = get_output('cd ' .. dir .. ' && git branch -a')
    print("Fs_LogInfo")
    for line in output:gmatch("[^\n]-\n") do
        if line:find("%s%->%s") then
            print(line:match("%s?%s?(.+)"))
            line = line:match("(.-)%s%->%s") .. '\n'
        end
        if line:find("^%*%s") then
            line = line:match("%s?%*?%s?(.+)")
            print(get_text("ENV_WFX_SCRIPT_STR_BRANCH") .. ' ' .. line)
        end
        treeish = treeish .. '\t' .. line:match("^%s?%s?(.+)\n")
    end
    local output = get_output('cd ' .. dir .. ' && git tag')
    for line in output:gmatch("[^\n]-\n") do
        treeish = treeish .. '\t' .. line:match("(.+)\n")
    end
    if treeish == extra then
        log_err(get_text("ENV_WFX_SCRIPT_STR_ERRTREEISH"))
        print("Fs_SelectDir WFX_SCRIPT_STR_REPO")
    end
    print("Fs_MultiChoice WFX_SCRIPT_STR_TREEISH" .. treeish)
end

function select_commit(dir, treeish, path, extra)
    if not extra then
        extra = ''
    end
    if path:len() > 1 then
        path = path:gsub('"', '\\"'):gsub('^/', '')
    else
        path = '.'
    end
    local output = get_output('cd ' .. dir .. ' && git log --oneline ' .. treeish .. extra .. ' -- "' .. path .. '"')
    local commits = ''
    for line in output:gmatch("[^\n]-\n") do
        commits = commits .. '\t' .. line:match("(.+)\n")
    end
    if commits == '' then
        log_err(get_text("ENV_WFX_SCRIPT_STR_ERRCOMMIT"))
        os.exit(1)
    end
    print("Fs_MultiChoice SHA1\t" .. commits)
    os.exit()
end

function show_reflog(dir)
    local output = get_output("cd " .. dir .. " && git reflog ")
    local commits = ''
    for line in output:gmatch("[^\n]-\n") do
        commits = commits .. '\t' .. line:match("(.+)\n")
    end
    if commits == '' then
        log_err(get_text("ENV_WFX_SCRIPT_STR_ERRREFLOG"))
        os.exit(1)
    end
    print("Fs_MultiChoice SHA1\t" .. commits)
    os.exit()
end

function where_am_i(dir, treeish)
    print("Fs_LogInfo")
    os.execute('cd ' .. dir .. ' && git describe --always ' .. treeish .. ' --')
    os.execute('cd ' .. dir .. ' && git log -1 --pretty=format:"%an (%ae)\n%ad\n%s\n%b" --date=format:"%Y-%m-%d %T" ' .. treeish .. ' --')
end

function diff_changes(dir, treeish, diff)
    local output = get_output('cd ' .. dir .. ' && git log -1 --pretty=format:%h ' .. treeish .. ' --')
    if not diff then
        print("Fs_ShowOutput git -C "..dir..' show --pretty="" -p ' .. output)
    else
        local extra = ' -p --pretty=""  -- > "' .. diff:gsub('"', '\\"') .. '"'
        os.execute('cd ' .. dir .. ' && git show ' .. output .. extra)
    end
end

function diff_head(dir, treeish, file, diff)
    local extra = ''
    if file ~= nil then
        extra = '"' .. file:gsub('^/', ''):gsub('"', '\\"') .. '" ":|*/" '
    end
    if not diff then
        print("Fs_ShowOutput " .. 'git -C ' .. dir .. ' diff "' .. treeish .. '..HEAD" -- ' .. extra)
    else
        extra = extra .. '> "' .. diff:gsub('"', '\\"') .. '"'
        os.execute('cd ' .. dir .. ' && git diff "' .. treeish .. '..HEAD" -- ' .. extra)
    end
end

function show_help()
    print("Fs_LogInfo")
    print(get_text("ENV_WFX_SCRIPT_STR_HELP"))
    print("\trepo\t\t\t" .. get_text("ENV_WFX_SCRIPT_STR_HELPREPO"))
    print("\tselect\t\t\t" .. get_text("ENV_WFX_SCRIPT_STR_HELPSEL"))
    print("\tset [tree-ish]\t\t" .. get_text("ENV_WFX_SCRIPT_STR_HELPHEAD"))
    print("\tcommits [..tree-ish]\t" .. get_text("ENV_WFX_SCRIPT_STR_HELPCOM"))
    print("\tadded\t\t\t" .. get_text("ENV_WFX_SCRIPT_STR_HELPADD"))
    print("\tchanges\t\t\t" .. get_text("ENV_WFX_SCRIPT_STR_HELPCHG"))
end

function fs_init()
    print("Fs_CONNECT_Needed")
    print("Fs_GetValue_Needed")
    print("Fs_SelectDir WFX_SCRIPT_STR_REPO")
    print("Fs_ClearValue WFX_SCRIPT_STR_DATEFORM")
    show_help()
    os.exit()
end

function fs_setopt(option, value)
    if option == "WFX_SCRIPT_STR_REPO" then
        local dir = '"' .. value:gsub('"', '\\"') .. '"'
        print("Fs_Set_DC_WFX_SCRIPT_DIR " .. dir)
        select_treeish(dir)
    elseif option == "WFX_SCRIPT_STR_TREEISH" or option == "WFX_SCRIPT_STR_CUSTOM" then
        if value == "WFX_SCRIPT_STR_CUSTOM" then
            print("Fs_Request_Options")
            print("WFX_SCRIPT_STR_CUSTOM")
            os.exit()
        end
        local dir = os.getenv("DC_WFX_SCRIPT_DIR")
        if value == "WFX_SCRIPT_STR_COMMIT" then
            select_commit(dir, "HEAD", '.', '')
            os.exit()
        elseif value == "WFX_SCRIPT_STR_REFLOG" then
            show_reflog(dir)
        end
        print("Fs_Set_DC_WFX_SCRIPT_TREEISH " .. value)
        where_am_i(dir, value)
    elseif option == "SHA1" then
        local dir = os.getenv("DC_WFX_SCRIPT_DIR")
        local commit = value:match("(.-)%s")
        print("Fs_Set_DC_WFX_SCRIPT_TREEISH " .. commit)
        where_am_i(dir, commit)
    elseif option == "WFX_SCRIPT_STR_DATES" then
        print("Fs_MultiChoice WFX_SCRIPT_STR_DATEFORM\tWFX_SCRIPT_STR_NONE\tWFX_SCRIPT_STR_ADATE\tWFX_SCRIPT_STR_CDATE")
    elseif option == "WFX_SCRIPT_STR_DATEFORM" then
        if value == "WFX_SCRIPT_STR_ADATE" then
            print("Fs_Set_DC_WFX_SCRIPT_DATES %ad")
        elseif value == "WFX_SCRIPT_STR_CDATE" then
            print("Fs_Set_DC_WFX_SCRIPT_DATES %cd")
        else
            print("Fs_Set_DC_WFX_SCRIPT_DATES ")
        end
    else
        local dir, treeish = get_data()
        if option == "WFX_SCRIPT_STR_CHG" then
            diff_changes(dir, treeish, nil)
        elseif option == "WFX_SCRIPT_STR_SELTREEISH" then
            select_treeish(dir)
        elseif option == "WFX_SCRIPT_STR_SELCOMMIT" then
            select_commit(dir, treeish, '.')
        elseif option == "WFX_SCRIPT_STR_SELCOMMITFILE" then
            select_commit(dir, treeish, value, '')
        elseif option == "WFX_SCRIPT_STR_CHGHEAD" then
            diff_head(dir, treeish, value, nil)
        elseif option == "WFX_SCRIPT_STR_DIFFCHG" then
            print("Fs_SelectFile WFX_SCRIPT_STR_DIFFCHGMSG\t*.diff\tdiff\tsave")
        elseif option == "WFX_SCRIPT_STR_DIFFHEAD" then
            print("Fs_SelectFile WFX_SCRIPT_STR_DIFFHEADMSG\t*.diff\tsave\tdiff")
        elseif option == "WFX_SCRIPT_STR_DIFFFILE" then
            print("Fs_SelectFile WFX_SCRIPT_STR_DIFFFILEMSG\t*.diff\tdiff\tsave")
        elseif option == "WFX_SCRIPT_STR_DIFFCHGMSG" then
            diff_changes(dir, treeish, value)
        elseif option == "WFX_SCRIPT_STR_DIFFHEADMSG" then
            diff_head(dir, treeish, nil, value)
        elseif option == "WFX_SCRIPT_STR_DIFFFILEMSG" then
            local file = os.getenv("DC_WFX_SCRIPT_REMOTENAME")
            diff_head(dir, treeish, file, value)
        elseif option == "WFX_SCRIPT_STR_OBJ" then
            print("Fs_LogInfo")
            local file = os.getenv("DC_WFX_SCRIPT_REMOTENAME")
            os.execute('cd ' .. dir .. ' && git ls-tree -l ' .. treeish .. file:gsub('"', '\\"'):gsub('^/', ' -- "') .. '"')
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
    local output = get_output('cd ' .. dir .. ' && git ls-tree -l ' .. treeish .. dirpath)
    for line in output:gmatch("[^\n]-\n") do
        local mode, objtype, objname, filesize, pathname = line:match('(%d+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^\n]+)\n')
        local datetime = nil
        local date_form = os.getenv("DC_WFX_SCRIPT_DATES")
        if date_form then
            datetime = get_output('cd ' .. dir .. ' && git log -1 --pretty=format:' .. date_form .. ' --date=format:"%Y-%m-%d %T" ' .. treeish .. ' -- "' .. pathname:gsub('"', '\\"') .. '"')
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
    os.execute('cd ' .. dir .. ' && git show "' .. treeish .. file:gsub('^/', ':'):gsub('"', '\\"') .. '" > "' .. target:gsub('"', '\\"') .. '"')
    os.exit()
end

function fs_properties(file)
    local dir, treeish = get_data()
    local output = get_output('cd ' .. dir .. ' && git ls-tree -l ' .. treeish .. file:gsub('"', '\\"'):gsub('^/', ' -- "') .. '"')
    local mode, objtype, objname, filesize, pathname = output:match('(%d+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+([^%s]+)%s+');
    if objtype == "tree" then
        print("content_type\tinode/directory")
    end
    output = get_output('cd ' .. dir .. ' && git log -1 --pretty=format:"%ad\n%an (%ae)\n%s" --date=format:"%Y-%m-%d %T" ' .. treeish .. file:gsub('"', '\\"'):gsub('^/', ' -- "') .. '"')
    local datetime, author, subject = output:match("([^\n]-)\n([^\n]-)\n(.+)$")
    print_prop("WFX_SCRIPT_STR_AUTHOR", author)
    print_prop("WFX_SCRIPT_STR_SUBJECT", subject)
    print_prop("WFX_SCRIPT_STR_ADATE", datetime)
    print_prop("WFX_SCRIPT_STR_SIZE", filesize)
    print_prop("WFX_SCRIPT_STR_MODE", mode)
    print_prop("Path", pathname)
    print_prop("WFX_SCRIPT_STR_PROPTREEISH", treeish)
    output = get_output('cd ' .. dir .. ' && git log -1 --pretty=format:"%cd\n%cn (%ce)\n%s" --date=format:"%Y-%m-%d %T" ' .. treeish)
    local datetime, commiter, subject = output:match("([^\n]-)\n([^\n]-)\n(.+)$")
    print_prop("WFX_SCRIPT_STR_COMMITER", commiter)
    print_prop("WFX_SCRIPT_STR_SUBJECT", subject)
    print_prop("WFX_SCRIPT_STR_CDATE", datetime)
    local acts = {
        "WFX_SCRIPT_STR_SELCOMMITFILE",
        "WFX_SCRIPT_STR_SELCOMMIT",
        "WFX_SCRIPT_STR_SELTREEISH",
        "WFX_SCRIPT_STR_CHGHEAD",
        "WFX_SCRIPT_STR_CHG",
        "WFX_SCRIPT_STR_DIFFHEAD",
        "WFX_SCRIPT_STR_DIFFFILE",
        "WFX_SCRIPT_STR_DIFFCHG",
        "WFX_SCRIPT_STR_OBJ",
        "WFX_SCRIPT_STR_DATES",
    }
    local propacts = "Fs_PropsActs"
    for i = 1, #acts do
        propacts = propacts .. '\t' .. acts[i]
    end
    print(propacts)
    os.exit()
end

function fs_quote(str, path)
    local dir, treeish = get_data()
    if str == "help" then
        show_help()
    elseif str == "repo" then
        print("Fs_SelectDir WFX_SCRIPT_STR_REPO")
    elseif str:find("^set") then
        treeish = str:match("^set%s*(.+)")
        if not treeish then
            treeish = "HEAD"
        end
        print("Fs_Set_DC_WFX_SCRIPT_TREEISH " .. treeish)
    elseif str:find("^commits") then
        select_commit(dir, treeish, path, str:match("^commits%s*(.+)"))
    elseif str == "select" then
        select_treeish(dir)
    elseif str == "changes" then
        diff_changes(dir, treeish, nil)
    elseif str == "added" then
        local pathspec = ""
        local offset = 3
        if path ~= '/' then
            offset = offset + path:len() - 1
            pathspec = '"' .. path:gsub('^/', ''):gsub('"', '\\"') .. '"'-- ":|*/"'
        end
        local output = get_output('cd ' .. dir .. ' && git diff --name-status "' .. treeish .. '..HEAD" --  ' .. pathspec)
        if not output then
            os.exit(1)
        end
        print("Fs_LogInfo")
        local added = 0
        for line in output:gmatch("[^\n]*\n?") do
            if line:find('^A') then
                print(line:sub(offset))
                added = added + 1
            end
        end
        if added > 0 then
            print('  ' .. added .. ' WFX_SCRIPT_STR_COUNT')
        end
    else
        os.exit(1)
    end
    os.exit()
end

function fs_getvalue(file)
    if file:find('/$') then
        os.exit(1)
    end
    local dir, treeish = get_data()
    local output = get_output('cd ' .. dir .. ' && git diff --name-status "' .. treeish .. '..HEAD" -- "' .. file:gsub('^/', ''):gsub('"', '\\"') .. '" ":|*/"')
    if not output then
        os.exit(1)
    end
    local added = 0
    local changed = 0
    local status = ''
    for line in output:gmatch("[^\n]*\n?") do
        if line:find('^A') then
            added = added + 1
        else
            changed = changed + 1
            if changed == 1 and line:len() > 0 then
                status = line:match(".")
            end
        end
    end
    if changed == 1 then
        print(status)
    elseif changed > 0 or added > 0 then
        if added > 0 then
            print(changed .. '+' .. added .. ' WFX_SCRIPT_STR_COUNT')
        end
        print(changed .. ' WFX_SCRIPT_STR_COUNT')
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
elseif args[1] == "getvalue" then
    fs_getvalue(args[2])
end

os.exit(1)
