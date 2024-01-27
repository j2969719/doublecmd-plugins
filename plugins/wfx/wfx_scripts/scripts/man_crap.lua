#!/bin/env lua

args = {...}
minsize = 150
groffx = false

function get_text(tepmlate)
    text = os.getenv(tepmlate)
    if not text then
        text = tepmlate
    end
    return text
end

mans = {
    [get_text("ENV_WFX_SCRIPT_STR_MAN0")] = "man0",
    [get_text("ENV_WFX_SCRIPT_STR_MAN0P")] = "man0p",
    [get_text("ENV_WFX_SCRIPT_STR_MAN1")] = "man1",
    [get_text("ENV_WFX_SCRIPT_STR_MAN1P")] = "man1p",
    [get_text("ENV_WFX_SCRIPT_STR_MAN2")] = "man2",
    [get_text("ENV_WFX_SCRIPT_STR_MAN2TYPE")] = "man2type",
    [get_text("ENV_WFX_SCRIPT_STR_MAN3")] = "man3",
    [get_text("ENV_WFX_SCRIPT_STR_MAN3CONST")] = "man3const",
    [get_text("ENV_WFX_SCRIPT_STR_MAN3HEAD")] = "man3head",
    [get_text("ENV_WFX_SCRIPT_STR_MAN3TYPE")] = "man3type",
    [get_text("ENV_WFX_SCRIPT_STR_MAN3P")] = "man3p",
    [get_text("ENV_WFX_SCRIPT_STR_MAN4")] = "man4",
    [get_text("ENV_WFX_SCRIPT_STR_MAN5")] = "man5",
    [get_text("ENV_WFX_SCRIPT_STR_MAN6")] = "man6",
    [get_text("ENV_WFX_SCRIPT_STR_MAN7")] = "man7",
    [get_text("ENV_WFX_SCRIPT_STR_MAN8")] = "man8",
    [get_text("ENV_WFX_SCRIPT_STR_MAN9")] = "man9",
    [get_text("ENV_WFX_SCRIPT_STR_MANL")] = "manl",
    [get_text("ENV_WFX_SCRIPT_STR_MANN")] = "mann",
    [get_text("ENV_WFX_SCRIPT_STR_MANX")] = "manx",
}

links = {
    [get_text("ENV_WFX_SCRIPT_STR_DOC")] = "/usr/share/doc/",
    [get_text("ENV_WFX_SCRIPT_STR_GTKDOC")] = "/usr/share/gtk-doc/html/",
    [get_text("ENV_WFX_SCRIPT_STR_DEVHELP")] = "/usr/share/devhelp/",
    [get_text("ENV_WFX_SCRIPT_STR_ARCHWIKI")] = "https://wiki.archlinux.org/",
    [get_text("ENV_WFX_SCRIPT_STR_GTK2")] = "https://developer-old.gnome.org/gtk2/stable/",
}

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

function log_err(str)
    if not str then
	return
    end
    if not str:find("\n$") then
        str = str .. '\n'
    end
    io.stderr:write(str)
end

function fs_init()
    print("Fs_PushValue WFX_SCRIPT_STR_MAN\tman man") -- rtfm
    print("Fs_RequestOnce")
    print("Fs_MultiChoice WFX_SCRIPT_STR_FRM\tWFX_SCRIPT_STR_TXT\tWFX_SCRIPT_STR_HTM\tWFX_SCRIPT_STR_PDF")
    os.exit()
end

function fs_setopt(option, value)
    if option == "WFX_SCRIPT_STR_FRM" and value == "WFX_SCRIPT_STR_PDF" then
        print("Fs_Set_DC_WFX_SCRIPT_DATA pdf")
    elseif option == "WFX_SCRIPT_STR_FRM" and value == "WFX_SCRIPT_STR_HTM" then
        print("Fs_Set_DC_WFX_SCRIPT_DATA html")
    elseif option == "WFX_SCRIPT_STR_FRM" then
        print("Fs_Set_DC_WFX_SCRIPT_DATA txt")
    elseif option == "WFX_SCRIPT_STR_MAN" then
        print('Fs_RunTermKeep ' .. value)
        --print("Fs_ShowOutput " .. value)
    end
    os.exit()
end

function fs_getlist(path)
    if path == '/' then
        for page in pairs(mans) do
            print("dr-xr-xr-x 0000-00-00 00:00:00 - " .. page)
        end
        for link in pairs(links) do
            print("lr-xr-xr-x 0000-00-00 00:00:00 - <" .. link .. ">.-->")
        end
        print("-r-xr-xr-x 0000-00-00 00:00:00 - >man<.run")
    else
        local ext = os.getenv("DC_WFX_SCRIPT_DATA")
        if not ext then
            ext = "txt"
        end
        local ls_output = get_output('ls -lA --full-time /usr/share/man/' .. mans[path:sub(2)])
        for line in ls_output:gmatch("[^\n]+") do
            local attr, size, datetime, name = line:match("([%-bcdflsrwxtST]+)%s+%d+%s+[%a%d_]+%s+[%a%d_]+%s+(%d+)%s+(%d%d%d%d%-%d%d%-%d%d%s%d%d:%d%d:%d%d)%.[%d]+%s[%-%+%d]+%s(.+)")
            if (attr ~= nil and datetime ~= nil and size ~= nil and name ~= nil) then
                name = name:gsub("%s%->%s.+$", "")
                if name:find("gz$") and tonumber(size) >= minsize then
                    print(attr .. '\t' .. datetime .. '\t-\t' .. name:gsub("gz$", ext))
                    --print(attr .. '\t' .. datetime .. '\t' .. size .. '\t' .. name:gsub("gz$", ext))
                end
            end
        end
    end
    os.exit()
end

function fs_getfile(src, dst)
    local cmd = nil
    local ext = src:match("%.([%a]-)$")
    if not ext or ext == "-->" then
        os.exit(1)
    end
    local page = src:match("([^/]+)%.[%a]-$")
    if (ext == "txt") then
        cmd = 'env LANG= man ' .. page .. ' > "' .. dst:gsub('"', '\\"') .. '"'
    else
        cmd = 'env LANG= man -T' .. ext .. ' ' .. page .. ' > "' .. dst:gsub('"', '\\"') .. '"'
    end
    if (cmd ~= nil) and (os.execute(cmd) == true) then
        os.exit()
    end
    os.exit(1)
end

function fs_execute(file)
    if file == "/>man<.run" then
        print("Fs_Request_Options\nWFX_SCRIPT_STR_MAN")
        os.exit()
    end
    local link = file:match("^/<([^>]+)>%.%-%->$")
    if link ~= nil and links[link] ~= nil then
        if links[link]:find("^http") then
            print("Fs_Open " .. links[link])
        else
            print(links[link])
        end
        os.exit()
    end
    if groffx == true then
        local ext = file:match("%.([%a]-)$")
        local filename = get_filename(file, ext)
        os.execute('gzip -cd ' .. filename .. ' | preconv | groff -mandoc -X &')
        os.exit()
    end
    os.exit(1)
end

function fs_properties(file)
    if file == "/>man<.run" then
        os.exit()
    end
    local link = file:match("^/<([^>]+)>%.%-%->$")
    if link ~= nil and links[link] ~= nil then
        print("Fs_Info_Message " .. links[link])
    elseif file:find("^/([^/]+)$") then
        print("Fs_Info_Message " .. mans[file:match("/([^/]+)")])
    else
   	    print("Fs_MultiChoice WFX_SCRIPT_STR_FRM\tWFX_SCRIPT_STR_TXT\tWFX_SCRIPT_STR_HTM\tWFX_SCRIPT_STR_PDF")
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
elseif args[1] == "run" then
    fs_execute(args[2])
elseif args[1] == "properties" then
    fs_properties(args[2])
end

os.exit(1)
