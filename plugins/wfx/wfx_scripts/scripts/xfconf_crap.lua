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

function fs_init()
  print("Fs_GetValues_Needed")
end

function fs_setopt(option, value)
  local path = option:match("^Fs_EditLine (.+)")
  if not path then
    os.exit(1)
  end
  local channel = path:match("/([^/]+)")
  local subpath = path:match("^/[^/]+(.+)")
  if not channel or not subpath then
    os.exit(1)
  end
  if (os.execute("xfconf-query -c " .. channel .. ' -p ' .. subpath .. ' -s "' .. value .. '"') == true) then
    os.exit()
  end
  os.exit(1)
end

function fs_getlist(path)
  if path == '/' then
    os.execute("xfconf-query -l | tail -n +2 | cut -c3- | sed 's/^/dr-xr-xr-x  0000-00-00 00:00:00 - /'")
  else
    local channel = path:match("/([^/]+)")
    local subpath = path:match("^/[^/]+/(.+)")
    if subpath ~= nil then
      subpath = subpath:gsub("%-", "%%-")
    end
    local dirs = {}
    local output = get_output("xfconf-query -l -c " .. channel .. " | cut -c2-")
    for line in output:gmatch("[^\n]+") do
      local name = line
      if subpath ~= nil then
        name = line:match('^' .. subpath .. "/(.+)")
      end
      if name ~= nil then
        if not name:find('/') then
          print("---------- 0000-00-00 00:00:00 - " .. name)
        else
          dirs[name:match("^[^/]+")] = ""
        end
      end
    end
    for dir in pairs(dirs) do
      print("d--------- 0000-00-00 00:00:00 - " .. dir)
    end
  end
  os.exit()
end

function fs_rm(file)
  local channel = file:match("/([^/]+)")
  local subpath = file:match("^/[^/]+(.+)")
  local command = "xfconf-query -r -R -c " .. channel
  if subpath ~= nil then
    command = command .. " -p " .. subpath
  end
  if (os.execute(command) == true) then
    os.exit()
  end
  os.exit(1)
end

function fs_run(file)
  local channel = file:match("/([^/]+)")
  local subpath = file:match("^/[^/]+(.+)")
  local output = get_output("xfconf-query -c " .. channel .. " -p " .. subpath)
  local _, line_count = output:gsub('\n', ' ')
  if line_count > 1 then
    print("Fs_ShowOutput xfconf-query -c " .. channel .. " -p " .. subpath)
  else
    print("Fs_EditLine " .. file .. '\t' .. output)
  end
  os.exit()
end

function fs_properties(file)
  local channel = file:match("/([^/]+)")
  local subpath = file:match("^/[^/]+(.+)")
  if subpath ~= nil then
    print("Fs_RunTermKeep xfconf-query -m -v -c " .. channel .. " -p " .. subpath)
  else
    print("Fs_RunTermKeep xfconf-query -m -v -c " .. channel)
  end
  os.exit()
end

function fs_getvalues(path)
  if path == '/' then
    os.exit(1)
  end
  local channel = path:match("/([^/]+)")
  local subpath = path:match("^/[^/]+/(.+)")
  if subpath ~= nil then
    subpath = subpath:gsub("%-", "%%-")
  end
  local output = get_output("xfconf-query -l -v -c " .. channel .. " | cut -c2-")
  for line in output:gmatch("[^\n]+") do
    local name = line:match("^([^%s]+)")
    if subpath ~= nil then
      name = name:match('^' .. subpath .. "/(.+)")
    end
    if name ~= nil and not name:find('/') then
      print(name .. '\t' .. line:match("^[^%s]+%s+(.+)"))
    end
  end
  os.exit()
end


if args[1] == "init" then
  fs_init(args[2])
elseif args[1] == "setopt" then
  fs_setopt(args[2], args[3])
elseif args[1] == "list" then
  fs_getlist(args[2])
elseif args[1] == "rm" then
  fs_rm(args[2])
elseif args[1] == "rmdir" then
  fs_rm(args[2])
elseif args[1] == "run" then
  fs_run(args[2])
elseif args[1] == "properties" then
  fs_properties(args[2])
elseif args[1] == "getvalues" then
  fs_getvalues(args[2])
end

os.exit(1)
