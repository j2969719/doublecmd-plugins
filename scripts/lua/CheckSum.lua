-- CheckSum.lua (cross-platform)
-- 2024.11.16
--
-- Calculate or verify checksum (auto choose):
-- if selected file is one of the DC supported (*.md5, *.sfv, *.sha and so on)
-- then will be run verification of checksum(s), in otherwise (or if selected
-- more then one files) will be run calculation of checksum(s).
--
-- Command:
--   cm_ExecuteScript
-- Parameters:
--   /path/to/script/CheckSum.lua
--   %"0%es
--   %"0%ps2

local params = {...}

function chk(e)
  local l = {
"blake2b", "blake2bp", "blake2s", "blake2sp", "blake3", "crc32", "haval",
"md4", "md5", "ripemd128", "ripemd160", "sfv", "sha", "sha224", "sha256",
"sha384", "sha512", "sha3", "tiger", "xxh128"
}
  e = string.lower(e)
  for i = 1, #l do
    if l[i] == e then return 1 end;
  end
  return 0;
end

if #params == 2 then
  local c = "cm_CheckSumCalc"
  if params[2] == '' then
    if chk(params[1]) == 1 then c = "cm_CheckSumVerify" end;
  end
  DC.ExecuteCommand(c)
end
