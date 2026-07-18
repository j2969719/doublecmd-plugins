#!/bin/env lua

local wcx = require("wcx_plug")

local plug = wcx.load_plug("/usr/lib/doublecmd/plugins/wcx/sevenzip/sevenzip.wcx")
if plug then
  print("pack_files", wcx.pack_files(plug, "/tmp/test.7z", ".", {"wcxtst.lua", "wdxtst.lua", "test.lua"}))
  print("delete_files", wcx.delete_files(plug, "/tmp/test.7z",  {"test.lua"}))
  print("probe", wcx.probe_archive(plug, "/tmp/test.7z"))
  print("walk_archive")
  for file, info in wcx.walk_archive(plug, "/tmp/test.7z", true) do
    print(info.attr, os.date("%Y-%m-%d %T", info.filetime), info.pk_size, info.unpk_size, file)
    if file == "wcxtst.lua" then
      print("extract_item", wcx.extract_item(plug, "/tmp/test1/extracted_script.lua"))
    end
  end
  print("extract_files", wcx.extract_files(plug, "*", "/tmp/test2", "/tmp/test.7z"))
  print("get_filelist")
  local filelist = wcx.get_filelist(plug, "/tmp/test.7z")
  for i=1, #filelist do
    print(filelist[i].file, filelist[i].info.attr)
  end
  wcx.unload_plug(plug)
end
local plug = wcx.load_plug("/usr/lib/doublecmd/plugins/wcx/zip/zip.wcx")
if plug then
  print("pack_files", wcx.pack_files(plug, "/tmp/test.zip", ".", {"wcxtst.lua", "wdxtst.lua", "test.lua"}))
  print("manual")
  if wcx.open_archive(plug, "/tmp/test.zip") then
    repeat
      local file, info = wcx.get_item(plug)
      if file then
        print("test", wcx.test_item(plug))
      end
    until not file
    wcx.close_archive(plug)
  end
  wcx.unload_plug(plug)
end