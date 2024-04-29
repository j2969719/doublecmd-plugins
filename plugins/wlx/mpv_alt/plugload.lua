function on_fileload()
  if mp.get_property("current-tracks/video") == nil then
    mp.commandv("expand-properties", "show-text", "${filtered-metadata}", 9999)
  else
    mp.commandv("show-text", "")
  end
  mp.set_property_bool("pause", false)
end

mp.set_property_bool("loop", true)
mp.register_event("file-loaded", on_fileload)
