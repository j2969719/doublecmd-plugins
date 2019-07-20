-- elfheaderinfo.lua (cross-platform)
-- 2019.07.20
--
-- Some info from ELF files.
-- Fields:
--  Class (32- or 64-bit)
--  Encoding (Little Endian or Big Enidan)
--  Machine (Architecture)
--  OS/ABI identification (OS- or ABI-specific ELF extensions)
--  Type (Object file type: relocatable, executable, PIE executable, shared object or core file)
--  IsExecutable (boolean: executable or PIE executable file)

local fields = {
 {"Class", "ELF32|ELF64", 7},
 {"Encoding", "Little Endian|Big Enidan", 7},
 {"Machine", "",  8},
 {"OS/ABI identification", "", 8},
 {"Type", "No file type|Relocatable file|Executable file|PIE executable file|Shared object file|Core file", 7},
 {"IsExecutable", "", 6}
}
local osabi = {
 [0x00] = "UNIX - System V",
 [0x01] = "Hewlett-Packard HP-UX",
 [0x02] = "NetBSD",
 [0x03] = "GNU/Linux",
 [0x04] = "GNU/Hurd",
 [0x05] = "86Open",
 [0x06] = "Sun Solaris",
 [0x07] = "AIX",
 [0x08] = "IRIX",
 [0x09] = "FreeBSD",
 [0x0a] = "Tru64 UNIX",
 [0x0b] = "Novell Modesto",
 [0x0c] = "OpenBSD",
 [0x0d] = "OpenVMS",
 [0x0e] = "Hewlett-Packard Non-Stop Kernel",
 [0x0f] = "Amiga Research OS",
 [0x10] = "Fenix OS",
 [0x11] = "Nuxi CloudABI",
 [0x12] = "Stratus Technologies OpenVOS",
 [0x61] = "ARM architecture",
 [0xca] = "Cafe OS"
}
--[[Reserved for future use: 0x0b - 0x0e, 0x0f, 0x1a - 0x23, 0x79 - 0x82, 0x91 - 0x9f
Reserved for future Intel use: 0xb6
Reserved for future ARM use: 0xb8
Reserved by Intel: 0xcd - 0xd1
Reserved for future use: 0xe1 - 0xf2, 0xf4 - 0xf6, 0xf8, 0xf9
]]
local machine = {
 [0x00] = "No machine",
 [0x01] = "AT&T WE 32100",
 [0x02] = "SPARC",
 [0x03] = "Intel 80386",
 [0x04] = "Motorola 68000",
 [0x05] = "Motorola 88000",
 [0x06] = "Intel MCU",
 [0x07] = "Intel 80860",
 [0x08] = "MIPS I Architecture",
 [0x09] = "IBM System/370 Processor",
 [0x0a] = "MIPS RS3000 Little-endian",

 [0x0f] = "Hewlett-Packard PA-RISC",

 [0x10] = "nCUBE",
 [0x11] = "Fujitsu VPP500",
 [0x12] = "Enhanced instruction set SPARC",
 [0x13] = "Intel 80960",
 [0x14] = "PowerPC",
 [0x15] = "64-bit PowerPC",
 [0x16] = "IBM System/390 Processor",
 [0x17] = "IBM SPU/SPC",
 [0x18] = "Cisco SVIP",
 [0x19] = "Cisco 7200",

 [0x24] = "NEC V800",
 [0x25] = "Fujitsu FR20",
 [0x26] = "TRW RH-32",
 [0x27] = "Motorola RCE",
 [0x28] = "ARM 32-bit architecture (AARCH32)",
 [0x29] = "Digital Alpha",
 [0x2a] = "Hitachi SH",
 [0x2b] = "SPARC Version 9",
 [0x2c] = "Siemens TriCore embedded processor",
 [0x2d] = "Argonaut RISC Core, Argonaut Technologies Inc.",
 [0x2e] = "Hitachi H8/300",
 [0x2f] = "Hitachi H8/300H",
 [0x30] = "Hitachi H8S",
 [0x31] = "Hitachi H8/500",
 [0x32] = "Intel IA-64 processor architecture",
 [0x33] = "Stanford MIPS-X",
 [0x34] = "Motorola ColdFire",
 [0x35] = "Motorola M68HC12",
 [0x36] = "Fujitsu MMA Multimedia Accelerator",
 [0x37] = "Siemens PCP",
 [0x38] = "Sony nCPU embedded RISC processor",
 [0x39] = "Denso NDR1 microprocessor",
 [0x3a] = "Motorola Star*Core processor",
 [0x3b] = "Toyota ME16 processor",
 [0x3c] = "STMicroelectronics ST100 processor",
 [0x3d] = "Advanced Logic Corp. TinyJ embedded processor family",
 [0x3e] = "AMD x86-64 architecture",
 [0x3f] = "Sony DSP Processor",
 [0x40] = "Digital Equipment Corp. PDP-10",
 [0x41] = "Digital Equipment Corp. PDP-11",
 [0x42] = "Siemens FX66 microcontroller",
 [0x43] = "STMicroelectronics ST9+ 8/16 bit microcontroller",
 [0x44] = "STMicroelectronics ST7 8-bit microcontroller",
 [0x45] = "Motorola MC68HC16 Microcontroller",
 [0x46] = "Motorola MC68HC11 Microcontroller",
 [0x47] = "Motorola MC68HC08 Microcontroller",
 [0x48] = "Motorola MC68HC05 Microcontroller",
 [0x49] = "Silicon Graphics SVx",
 [0x4a] = "STMicroelectronics ST19 8-bit microcontroller",
 [0x4b] = "Digital VAX",
 [0x4c] = "Axis Communications 32-bit embedded processor",
 [0x4d] = "Infineon Technologies 32-bit embedded processor",
 [0x4e] = "Element 14 64-bit DSP Processor",
 [0x4f] = "LSI Logic 16-bit DSP Processor",
 [0x50] = "Donald Knuth's educational 64-bit processor",
 [0x51] = "Harvard University machine-independent object files",
 [0x52] = "SiTera Prism",
 [0x53] = "Atmel AVR 8-bit microcontroller",
 [0x54] = "Fujitsu FR30",
 [0x55] = "Mitsubishi D10V",
 [0x56] = "Mitsubishi D30V",
 [0x57] = "NEC v850",
 [0x58] = "Mitsubishi M32R",
 [0x59] = "Matsushita MN10300",
 [0x5a] = "Matsushita MN10200",
 [0x5b] = "picoJava",
 [0x5c] = "OpenRISC 32-bit embedded processor",
 [0x5d] = "ARC International ARCompact processor",
 [0x5e] = "Tensilica Xtensa Architecture",
 [0x5f] = "Alphamosaic VideoCore processor",
 [0x60] = "Thompson Multimedia General Purpose Processor",
 [0x61] = "National Semiconductor 32000 series",
 [0x62] = "Tenor Network TPC processor",
 [0x63] = "Trebia SNP 1000 processor",
 [0x64] = "STMicroelectronics ST200 microcontroller",
 [0x65] = "Ubicom IP2xxx microcontroller family",
 [0x66] = "MAX Processor",
 [0x67] = "National Semiconductor CompactRISC microprocessor",
 [0x68] = "Fujitsu F2MC16",
 [0x69] = "Texas Instruments embedded microcontroller msp430",
 [0x6a] = "Analog Devices Blackfin (DSP) processor",
 [0x6b] = "S1C33 Family of Seiko Epson processors",
 [0x6c] = "Sharp embedded microprocessor",
 [0x6d] = "Arca RISC Microprocessor",
 [0x6e] = "Microprocessor series from PKU-Unity Ltd. and MPRC of Peking University",
 [0x6f] = "eXcess: 16/32/64-bit configurable embedded CPU",
 [0x70] = "Icera Semiconductor Inc. Deep Execution Processor",
 [0x71] = "Altera Nios II soft-core processor",
 [0x72] = "National Semiconductor CompactRISC CRX microprocessor",
 [0x73] = "Motorola XGATE embedded processor",
 [0x74] = "Infineon C16x/XC16x processor",
 [0x75] = "Renesas M16C series microprocessors",
 [0x76] = "Microchip Technology dsPIC30F Digital Signal Controller",
 [0x77] = "Freescale Communication Engine RISC core",
 [0x78] = "Renesas M32C series microprocessors",

 [0x83] = "Altium TSK3000 core",
 [0x84] = "Freescale RS08 embedded processor",
 [0x85] = "Analog Devices SHARC family of 32-bit DSP processors",
 [0x86] = "Cyan Technology eCOG2 microprocessor",
 [0x87] = "Sunplus S+core7 RISC processor",
 [0x88] = "New Japan Radio (NJR) 24-bit DSP Processor",
 [0x89] = "Broadcom VideoCore III processor",
 [0x8a] = "RISC processor for Lattice FPGA architecture",
 [0x8b] = "Seiko Epson C17 family",
 [0x8c] = "The Texas Instruments TMS320C6000 DSP family",
 [0x8d] = "The Texas Instruments TMS320C2000 DSP family",
 [0x8e] = "The Texas Instruments TMS320C55x DSP family",
 [0x8f] = "Texas Instruments Application Specific RISC Processor, 32bit fetch",
 [0x90] = "Texas Instruments Programmable Realtime Unit",

 [0xa0] = "STMicroelectronics 64bit VLIW Data Signal Processor",
 [0xa1] = "Cypress M8C microprocessor",
 [0xa2] = "Renesas R32C series microprocessors",
 [0xa3] = "NXP Semiconductors TriMedia architecture family",
 [0xa4] = "QUALCOMM DSP6 Processor",
 [0xa5] = "Intel 8051 and variants",
 [0xa6] = "STMicroelectronics STxP7x family of configurable and extensible RISC processors",
 [0xa7] = "Andes Technology compact code size embedded RISC processor family",
 [0xa8] = "Cyan Technology eCOG1X family",
 [0xa9] = "Dallas Semiconductor MAXQ30 Core Micro-controllers",
 [0xaa] = "New Japan Radio (NJR) 16-bit DSP Processor",
 [0xab] = "M2000 Reconfigurable RISC Microprocessor",
 [0xac] = "Cray Inc. NV2 vector architecture",
 [0xad] = "Renesas RX family",
 [0xae] = "Imagination Technologies META processor architecture",
 [0xaf] = "MCST Elbrus general purpose hardware architecture",
 [0xb0] = "Cyan Technology eCOG16 family",
 [0xb1] = "National Semiconductor CompactRISC CR16 16-bit microprocessor",
 [0xb2] = "Freescale Extended Time Processing Unit",
 [0xb3] = "Infineon Technologies SLE9X core",
 [0xb4] = "Intel L10M",
 [0xb5] = "Intel K10M",

 [0xb7] = "ARM 64-bit architecture (AARCH64)",

 [0xb9] = "Atmel Corporation 32-bit microprocessor family",
 [0xba] = "STMicroeletronics STM8 8-bit microcontroller",
 [0xbb] = "Tilera TILE64 multicore architecture family",
 [0xbc] = "Tilera TILEPro multicore architecture family",
 [0xbd] = "Xilinx MicroBlaze 32-bit RISC soft processor core",
 [0xbe] = "NVIDIA CUDA architecture",
 [0xbf] = "Tilera TILE-Gx multicore architecture family",
 [0xc0] = "CloudShield architecture family",
 [0xc1] = "KIPO-KAIST Core-A 1st generation processor family",
 [0xc2] = "KIPO-KAIST Core-A 2nd generation processor family",
 [0xc3] = "Synopsys ARCompact V2",
 [0xc4] = "Open8 8-bit RISC soft processor core",
 [0xc5] = "Renesas RL78 family",
 [0xc6] = "Broadcom VideoCore V processor",
 [0xc7] = "Renesas 78KOR family",
 [0xc8] = "Freescale 56800EX Digital Signal Controller (DSC)",
 [0xc9] = "Beyond BA1 CPU architecture",
 [0xca] = "Beyond BA2 CPU architecture",
 [0xcb] = "XMOS xCORE processor family",
 [0xcc] = "Microchip 8-bit PIC(r) family",

 [0xd2] = "KM211 KM32 32-bit processor",
 [0xd3] = "KM211 KMX32 32-bit processor",
 [0xd4] = "KM211 KMX16 16-bit processor",
 [0xd5] = "KM211 KMX8 8-bit processor",
 [0xd6] = "KM211 KVARC processor",
 [0xd7] = "Paneve CDP architecture family",
 [0xd8] = "Cognitive Smart Memory Processor",
 [0xd9] = "Bluechip Systems CoolEngine",
 [0xda] = "Nanoradio Optimized RISC",
 [0xdb] = "CSR Kalimba architecture family",
 [0xdc] = "Zilog Z80",
 [0xdd] = "Controls and Data Services VISIUMcore processor",
 [0xde] = "FTDI Chip FT32 high performance 32-bit RISC architecture",
 [0xdf] = "Moxie processor family",
 [0xe0] = "AMD GPU architecture",

 [0xf3] = "RISC-V",

 [0xf7] = "Linux BPF - in-kernel virtual machine",

 [0xfa] = "C-SKY"
}

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1][1], fields[FieldIndex + 1][2], fields[FieldIndex + 1][3]
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1
end

function ContentGetDetectString()
  return 'EXT="*"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  local at = SysUtils.FileGetAttr(FileName)
  if (at < 0) or (math.floor(at / 0x00000004) % 2 ~= 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then
    return nil
  end
  local f = io.open(FileName, 'rb')
  fc = f:read(62)
  f:close()
  if fc == nil then return nil end
  if (string.byte(fc, 1) == 0x7f) and (string.byte(fc, 2) == 0x45) and (string.byte(fc, 3) == 0x4c) and (string.byte(fc, 4) == 0x46) then
    if FieldIndex == 0 then
      local ei_class = string.byte(fc, 5)
      if ei_class == 0x01 then
        return "ELF32"
      elseif ei_class == 0x02 then
        return "ELF64"
      end
    elseif FieldIndex == 1 then
      local ei_data = string.byte(fc, 6)
      if ei_data == 0x01 then
        return "Little Endian"
      elseif ei_data == 0x02 then
        return "Big Enidan"
      end
    elseif FieldIndex == 2 then
      local ei_data = string.byte(fc, 6)
      local n1, n2
      if ei_data == 0x01 then
        n1, n2 = 20, 19
      elseif ei_data == 0x02 then
        n1, n2 = 19, 20
      else
        return nil
      end
      if string.byte(fc, n1) ~= 0x00 then return nil end
      return machine[string.byte(fc, n2)]
    elseif FieldIndex == 3 then
      return osabi[string.byte(fc, 8)]
    elseif (FieldIndex == 4) or (FieldIndex == 5) then
      local ei_data = string.byte(fc, 6)
      local n1, n2
      if ei_data == 0x01 then
        n1, n2 = 18, 17
      elseif ei_data == 0x02 then
        n1, n2 = 17, 18
      else
        return nil
      end
      if string.byte(fc, n1) ~= 0x00 then return nil end
      local ie = false
      local e_type = string.byte(fc, n2)
      if e_type == 0x00 then
        r = "No file type"
      elseif e_type == 0x01 then
        r = "Relocatable file"
      elseif e_type == 0x02 then
        r = "Executable file"
        ie = true
      elseif e_type == 0x03 then
        local ip = IsPIE(FileName, fc, ei_data)
        if ip == true then
          r = "PIE executable file"
          ie = true
        elseif ip == false then
          r = "Shared object file"
        else
          return nil
        end
      elseif e_type == 0x04 then
        r = "Core file"
      end
      if FieldIndex == 4 then return r else return ie end
    end
  end
  return nil
end

function BinToHex(d, n1, n2, e)
  local r = ''
  if e == 0x01 then
    for j = n1, n2 do r = ToHex(string.byte(d, j)) .. r end
  else
    for j = n1, n2 do r = r .. ToHex(string.byte(d, j)) end
  end
  return tonumber('0x' .. r)
end

function ToHex(n)
  n = string.format('%x', n)
  if string.len(n) == 1 then return '0' .. n else return n end
end

function IsPIE(FileName, fh, ei_data)
  local n1, n2
  local ei_class = string.byte(fh, 5)
  if ei_class == 0x01 then
    n1, n2 = 29, 32
  elseif ei_class == 0x02 then
    n1, n2 = 33, 40
  else
    return nil
  end
  local e_phoff = BinToHex(fh, n1, n2, ei_data)
  if e_phoff == 0 then return "Shared object file" end
  if ei_class == 0x01 then
    n1, n2 = 45, 46
  else
    n1, n2 = 57, 58
  end
  local e_phnum = BinToHex(fh, n1, n2, ei_data)
  if ei_class == 0x01 then
    n1, n2 = 43, 44
  else
    n1, n2 = 55, 56
  end
  local e_phentsize = BinToHex(fh, n1, n2, ei_data)
  local f, s, rd, p_type
  f = io.open(FileName, 'rb')
  s = f:seek('set', e_phoff)
  if s == nil then
    f:close()
    return nil
  end
  for i = 1, e_phnum do
    rd = f:read(e_phentsize)
    p_type = BinToHex(rd, 1, 4, ei_data)
    if p_type == 2 then break end
  end
  if p_type ~= 2 then
    f:close()
    return false
  else
   -- PT_DYNAMIC
    if ei_class == 0x01 then
      n1, n2 = 5, 8
    else
      n1, n2 = 9, 16
    end
    local p_offset = BinToHex(rd, n1, n2, ei_data)
    if ei_class == 0x01 then
      n1, n2 = 17, 20
    else
      n1, n2 = 33, 40
    end
    local p_filesz = BinToHex(rd, n1, n2, ei_data)
    s = f:seek('set', p_offset)
    if s == nil then
      f:close()
      return nil
    end
    rd = f:read(p_filesz)
    f:close()
    local l, t
    if ei_class == 0x01 then l = 8 else l = 16 end
    for i = 1, p_filesz - l, l do
      t = BinToHex(rd, i, i + (l / 2) - 1, ei_data)
      -- DT_FLAGS_1 == 0x6ffffffb
      if t == 0x6ffffffb then
        t = BinToHex(rd, i + (l / 2), i + l - 1, ei_data)
        -- DF_1_PIE = 0x08000000
        if math.floor(t / 134217728) % 2 ~= 0 then return true end
      end
    end
  end
  return false
end
