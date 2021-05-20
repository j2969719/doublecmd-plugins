#!/bin/env lua

-- In Poettering We Trust

args = {...}
os.setlocale("C")
env_var = 'DC_WFX_TP_SCRIPT_DATA'
restart_msg = "Do you want to restart the "
connect_txt = "Connect to"
system_txt = "System Manager"
user_txt = "User Service Manager"

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

function prepare_command(command)
    data = os.getenv(env_var)
    if not data then
        return command
    else
        return command .. data
    end
end

function fs_init()
    print("Fs_MultiChoice " .. connect_txt .."\t" .. system_txt .."\t" .. user_txt)
    os.exit()
end

function fs_setopt(option, value)
    if option:find(restart_msg) and value == "Yes" then
        service = option:gsub(restart_msg, ''):sub(1, -2)
        command = prepare_command("systemctl restart " .. service)
        os.execute(command)
    elseif option == connect_txt then
        if value == user_txt then
            print("Fs_Set_" .. env_var .. "  --user ")
        elseif value == system_txt then
            print("Fs_Set_" .. env_var .. "  --system ")
        end
    end
    os.exit()
end

function fs_getlist(path)
    if path == "/" then
        local states = {
            "running",
            "exited",
            "dead",
            "failed",
            "active",
            "inactive",
        }
        for _, state in pairs(states) do
            print("dr-xr-xr-x 0000-00-00 00:00:00 - " .. state)
        end
    else
        if not path:find("/$") then
            path = path:sub(2, -1)
        else
            path = path:sub(2, -2)
        end
        local command = prepare_command("systemctl list-units --legend=false --no-pager --state=" .. path)
        local output = get_output(command)
        for service in output:gmatch(".%s([^%s]+)%s+%w+%s+%w+%s+%w+%s+[^\n]-\n") do
            local size = nil
            local datetime = nil
            local fileinfo = "-r--r--r-- "
            command = prepare_command("systemctl status " .. service)
            local status = get_output(command)
            local active = status:match("\n%s+Active:%s([^\n]-)\n")
            local memory = status:match("\n%s+Memory:%s([^\n]-)\n")
            if active ~= nil then
                datetime = active:match("%d%d%d%d%-%d%d%-%d%d%s%d%d:%d%d:%d%d")
            end
            if datetime ~= nil then
                fileinfo = fileinfo .. datetime
            else
                fileinfo = fileinfo .. "0000-00-00 00:00:00"
            end
            fileinfo = fileinfo .. "\t"
            if memory ~= nil then
                --
                value = tonumber(memory:match("%d+.%d"))
                local char = memory:sub(-1)
                if char == 'B' then
                    size = value
                elseif char == 'K' then
                    size = math.floor(value * 1024)
                elseif char == 'M' then
                    size = math.floor(value * math.pow(1024, 2))
                elseif char == 'G' then
                    size = math.floor(value * math.pow(1024, 3))
                elseif char == 'T' then
                    size = math.floor(value * math.pow(1024, 4))
                end
            end
            if size ~= nil then
                fileinfo = fileinfo .. size
            else
                fileinfo = fileinfo .. "0"
            end
            fileinfo = fileinfo .. "\t"
            print(fileinfo .. service)
        end
    end
    os.exit()
end

function fs_getfile(file, target)
    local command = prepare_command("systemctl status " .. file:match("([^/]+)$"))
    os.execute(command .. ' > "' .. target:gsub('"', '\\"') .. '"')
    os.exit()
end

function fs_properties(file)
    local fields = {
        "Loaded",
        "Active",
        "Docs",
        "Main PID",
        "Status",
        "Tasks",
        "Memory",
        "CPU"
    }
    local command = prepare_command("systemctl status " .. file:match("([^/]+)$"))
    local output = get_output(command)
    for _, field in pairs(fields) do
        value = output:match("\n%s+" .. field .. ":%s([^\n]-)\n")
        if value ~= nil then
            print(field..'\t'..value)
        end
    end
    os.exit()
end

function fs_execute(file)
    if file:find("/failed/") then
        print("Fs_YesNo_Message " .. restart_msg .. file:match("([^/]+)$") .. '?')
        os.exit()
    end
    os.exit(1)
end

function fs_getlocalname(file)
    local command = prepare_command("systemctl status " .. file:match("([^/]+)$"))
    local output = get_output(command)
    local name = output:match("\n%s+Loaded:%s%w+%s%(([^;]-);")
    if name ~= nil then
        local file = io.open(name)
        if file ~= nil then
            file:close()
            print(name)
        end
        os.exit()
    end
    os.exit(1)
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
