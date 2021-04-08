#!/bin/env lua
--[[
⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣶⣄⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣦⣄⣀⡀⣠⣾⡇⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀
⠀⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⠿⢿⣿⣿⡇⠀⠀⠀⠀
⠀⣶⣿⣦⣜⣿⣿⣿⡟⠻⣿⣿⣿⣿⣿⣿⣿⡿⢿⡏⣴⣺⣦⣙⣿⣷⣄⠀⠀⠀
⠀⣯⡇⣻⣿⣿⣿⣿⣷⣾⣿⣬⣥⣭⣽⣿⣿⣧⣼⡇⣯⣇⣹⣿⣿⣿⣿⣧⠀⠀
⠀⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠸⣿⣿⣿⣿⣿⣿⣿⣷⠀
]]

args = {...}

function fs_init()
  print("some_option")
  os.exit()
end

function fs_setopts(option, value)
  if value ~= nil then
    print("let's pretend i dont care about " .. value)
    os.exit()
  end
  os.exit(1)
end

function fs_getlist(path)
  if path ~= "/" then
    os.exit(1)
  end
  print("-rw-r--r-- 2021-04-08T08:59:34Z   666  wishmaster.exe")
  print("drwxr-xr-x 2021-04-08T08:59:34Z   -   the path of a philosopher")
  os.exit()
end

function fs_getfile(file, target)
  if file == "/wishmaster.exe" and target ~= nil then
    os.execute("echo kek > " .. target)
    os.exit()
  end
  os.exit(1)
end

if args[1] == "init" then
  fs_init()
elseif args[1] == "setopt" then
  fs_setopts(args[2], args[3])
elseif args[1] == "list" then
  fs_getlist(args[2])
elseif args[1] == "copyout" then
  fs_getfile(args[2], args[3])
end

os.exit(1)