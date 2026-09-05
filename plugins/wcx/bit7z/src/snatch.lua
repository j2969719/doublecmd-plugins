#!/bin/env lua

for line in io.lines("./third_party/bit7z/include/bit7z/bitformat.hpp") do
  frmt, name = line:match("extern const BitIn%w+ (%w+);%s+///< (%w+)")
  if frmt and name then
    print('\t{"'..name..'",\tBitFormat::'..frmt..'},')
  end
end