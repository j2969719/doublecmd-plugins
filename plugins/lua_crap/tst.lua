#!/bin/env lua

local shiet = require("random_shiet")

print(shiet.which("7z"))
--shiet.say(shiet.which("7z"))
print("copy_file /etc/lsb-release -> /tmp/lsb-release", shiet.copy_file("/etc/lsb-release", "/tmp/lsb-release"))
print("trash_file /tmp/lsb-release", shiet.trash_file("/tmp/lsb-release"))
print()
local props = shiet.grab_props("/tmp/")
for key, value in pairs(props) do
  --shiet.say(key, value)
  print(key, value)
end
print()
print("wow mode is " .. string.format("%o", props["unix::mode"] % 512))
if not props["access::can-rename"] then
  print("cannot rename it")
end
print(shiet.human_size(props["standard::size"]))
print()
--print(shiet.get_output("ls -lA ."))
for file, filetype in shiet.list_dir(".") do
  print(file, filetype)
end
print()
print(shiet.user_cache_dir)
print(shiet.user_data_dir)
print(shiet.user_config_dir)
print(shiet.user_state_dir)
print(shiet.user_runtime_dir)
