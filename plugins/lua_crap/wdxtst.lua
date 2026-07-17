#!/bin/env lua

local wdx = require("wdx_plug")

local plug = wdx.load_plug("../wdx/exiftoolwdx_c/exiftoolwdx_c.wdx")
if plug then
  print(wdx.detectstring(plug))
  print("get_value CPU Type", wdx.get_value(plug, "./wdx_plug.so", "CPU Type", nil))
  print()
  local dump = wdx.get_values(plug, "./wdx_plug.so")
  for key, value in pairs(dump) do
    print(key, value)
  end
  wdx.unload_plug(plug)
end

plug = wdx.load_plug("../wdx/textline/textline.wdx")
if plug then
  print()
  local fields = wdx.get_fields(plug)

  for i = 1, #fields do
    if not fields[i].units then
      print(fields[i].name, fields[i].type)
    else
      local units = ''
      for n = 1, #fields[i].units do
        units = units .. '\t' .. fields[i].units[n]
      end
      print(fields[i].name, fields[i].type, units)
    end
  end
  print()
  print("get_indexes fieldname '5', unit 'OEM' >>>", wdx.get_indexes(plug, "5", "OEM"))
  print()
  print(wdx.detectstring(plug))
  local dump = wdx.get_values(plug, "./test.lua")
  for key, value in pairs(dump) do
    print(key, value)
  end
  wdx.unload_plug(plug)
end

plug = wdx.load_plug("../wdx/poppler_info/poppler_info.wdx")
if plug then
  print()
  print(wdx.get_value(plug, "weima-manual-petrol-engines.pdf", "Text"))
  wdx.unload_plug(plug)
end