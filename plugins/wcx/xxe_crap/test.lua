#!/bin/env lua

package.cpath = package.cpath .. ";../../lua_crap/?.so"
wcx = require("wcx_plug")

local plug = wcx.load_plug("./xxe.wcx")
if plug then
  print("pack", wcx.pack_files(plug, "/tmp/test.xxe", ".", {"test.lua"}))
  print("filelist")
  for file, info in wcx.walk_archive(plug, "/tmp/test.xxe", true) do
    print(string.format("%o %d %d %s", info.mode, info.pk_size, info.unpk_size, file))
    print("test file", wcx.test_item(plug))
  end
  print("extract_files", wcx.extract_files(plug, "*", "/tmp/test", "/tmp/test.xxe"))
end