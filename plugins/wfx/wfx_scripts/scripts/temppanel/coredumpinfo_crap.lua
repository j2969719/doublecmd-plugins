#!/bin/env lua

-- In Poettering We Trust 2

args = {...}
terminal_cmd = "xterm -e"  -- xterm ftw

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

function fs_init()
    print("Fs_PushValue WFX_SCRIPT_STR_TERM\t" .. terminal_cmd)
    os.exit()
end

function fs_setopt(option, value)
    if option == "WFX_SCRIPT_STR_TERM" and value ~= '' then
        print("Fs_Set_DC_WFX_TP_SCRIPT_DATA " .. value)
    end
    os.exit()
end

function fs_getlist(path)
    if path == "/" then
        local output = get_output("coredumpctl list")
        for line in output:gmatch("[^\n]-\n") do
            local filedate, pid, name, size = line:match("^%w+%s([%-%d]+%s[:%d]+)%s[%-%+%d]+%s+(%d+)%s+%d+%s+%d+%s%w+%s%w+%s+(.+)%s+([naMKG%d%./]+)%s*$")
            if filedate ~= nil and pid ~= nil and name ~= nil then
                local filesize = nil
                if size ~= nil then
                    filesize = tonumber(size:match("%d+.%d"))
                    local char = size:sub(-1)
                    if char == 'K' then
                        filesize = math.floor(filesize * 1024)
                    elseif char == 'M' then
                        filesize = math.floor(filesize * math.pow(1024, 2))
                    elseif char == 'G' then
                        filesize = math.floor(filesize * math.pow(1024, 3))
                    end
                end
                if filesize == nil then
                    filesize = '-'
                end
                print("-r-xr-xr-x " .. filedate .. " " .. filesize .. " " .. name:gsub("%s+$", ""):match("[^/]+$") .. "." .. pid)
            end
        end
    end
    os.exit()
end

function fs_getfile(file, target)
    os.execute('coredumpctl dump ' .. file:match("%.(%d+)$") .. ' > "' .. target:gsub('"', '\\"') .. '"')
    os.exit()
end

function fs_properties(file)
    local fields = {
        "WFX_SCRIPT_STR_CMD",
        "WFX_SCRIPT_STR_EXE",
        "WFX_SCRIPT_STR_SIG",
        "WFX_SCRIPT_STR_PATH",
        "WFX_SCRIPT_STR_MSG",
        "WFX_SCRIPT_STR_TIME",
        "WFX_SCRIPT_STR_SIZE",
    }
    local patterns = {
        "Command Line:%s([^\n]-)\n",
        "Executable:%s([^\n]-)\n",
        "Signal:%s([^\n]-)\n",
        "Storage:%s([^\n]-)\n",
        "Message:%s([^\n]-)\n",
        "Timestamp:%s([^\n]-)\n",
        "Disk Size:%s([^\n]-)\n",
    }
    local output = get_output('coredumpctl info ' .. file:match('%.(%d+)$'))
    for i = 1, #fields do
        value = output:match(patterns[i])
        if value ~= nil then
            print(fields[i]..'\t'..value)
        end
    end
    print('content_type\tapplication/x-core')
    os.exit()
end

function fs_execute(file)

    local data = os.getenv("DC_WFX_TP_SCRIPT_DATA")
    if data ~= nil and data ~= '' then
        terminal_cmd = data
    end
    os.execute(terminal_cmd .. ' "coredumpctl debug ' .. file:match("%.(%d+)$") .. '"')
    os.exit()
end

function fs_getlocalname(file)
    local pid = file:match("%.(%d+)$")
    os.execute('coredumpctl info ' .. pid .. ' > /tmp/coredumpinfo_crap.' .. pid)
    print('/tmp/coredumpinfo_crap.' .. pid)
    os.exit()
end


if args[1] == 'init' then
    fs_init()
elseif args[1] == 'setopt' then
    fs_setopt(args[2], args[3])
elseif args[1] == 'list' then
    fs_getlist(args[2])
elseif args[1] == 'copyout' then
    fs_getfile(args[2], args[3])
elseif args[1] == "run" then
    fs_execute(args[2])
elseif args[1] == 'properties' then
    fs_properties(args[2])
elseif args[1] == 'localname' then
    fs_getlocalname(args[2])
elseif args[1] == 'deinit' then
    os.execute('rm /tmp/coredumpinfo_crap.*')
end

os.exit(1)
