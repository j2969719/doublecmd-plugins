function dump_meta()
  mp.commandv("expand-properties", "show-text", "${metadata}", 9999)
end

mp.register_event('file-loaded', dump_meta)
