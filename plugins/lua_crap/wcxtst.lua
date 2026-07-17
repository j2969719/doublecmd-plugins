#!/bin/env lua

local wcx = require("wcx_plug")

--local plug = wcx.load_plug("./zip.wcx")
local plug = wcx.load_plug("./sevenzip.wcx")
if plug then
  for file, info in wcx.list_archive(plug, "zip.7z", true) do
    print(info.mode, os.date("%Y-%m-%d %T", info.filetime), info.pk_size, info.unpk_size, file)
    if file == "zip/zip.wcx" then
      print(wcx.snatch_current(plug, "data.bin"))
    end
  end
  wcx.unload_plug(plug)
end

local plug = wcx.load_plug("./unrar.wcx")
if plug then
  for file, info in wcx.list_archive(plug, "unrar.rar", true) do
    print(info.mode, os.date("%Y-%m-%d %T", info.filetime), info.pk_size, info.unpk_size, file)
    if file == "unrar/readme.txt" then
      print(wcx.snatch_current(plug, "data.txt"))
    end
  end
  wcx.unload_plug(plug)
end