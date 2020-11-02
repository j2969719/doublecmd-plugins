-- ziminfowdx.lua (cross-platform)
-- 2020.11.02
--[[
Save as UTF-8 without BOM!

Get some info from Zim file (https://zim-wiki.org/)
Supported fields: see table "fields".

Field "Creation date" is optional, from second line of the body (if exists):
  "Created [day_of_the_week] [day_of_the_month] [full_month_name] [year]"
  List of month (http://www.webteka.com/months-in-many-languages/):
  Afrikaans, Belarusian, Bulgarian, Czech and Czech (modern), Danish, Dutch, English,
  Estonian, Finnish, French, German, Greek, Hungarian, Icelandic, Indonesian, Italian,
  Latvian, Lithuanian, Norwegian, Polish, Portuguese, Romanian, Russian, Serbian,
  Slovak, Slovenian, Spanish, Swedish, Turkish, Ukrainian,
]]

local fields = {
"Header: Wiki-Format",
"Header: Creation-Date",
"Note title",
"Creation date",
"Note address",
"Notebook file",
"Notebook folder",
"Notebook name"
}
local head = {}
local body = {}
local month1 = {
{"tamm"},
{"helm"},
{"maal"},
{"huht"},
{"touk"},
{"ioún","juin","juun","kesä","ιούν"},
{"hein","ioúl","juil","juul","ιούλ"},
{"elok"},
{"syys"},
{"loka"},
{"kası","marr"},
{"joul"}
}
local month2 = {
{"ene","gen","ian","jaa","jan","led","oca","sau","sic","stu","sty","ιαν","сту","січ","янв","яну","јан"},
{"feb","fev","fév","lut","lyu","vas","vee","úno","şub","φεβ","лют","феб","фев"},
{"ber","bře","kov","maa","mar","már","mär","sak","μάρ","бер","мар","сак"},
{"abr","apr","avr","bal","dub","kra","kvi","kwi","nis","ápr","απρ","апр","кві","кра"},
{"geg","kvě","mag","mai","maj","may","maí","mei","mái","máj","tra","μάι","май","мая","мај","тра"},
{"bir","che","cze","giu","haz","iun","jun","jún","jūn","июн","чер","чэр","юни","јун"},
{"iul","jul","júl","jūl","lie","lip","lug","lyp","tem","июл","лип","ліп","юли","јул"},
{"ago","agu","aoû","aug","avg","aúg","ağu","rug","ser","sie","srp","zhn","ágú","αύγ","авг","жні","сер"},
{"eyl","rus","sep","set","sze","ver","vie","wrz","zář","σεπ","вер","сен","сеп"},
{"eki","kas","oct","okt","ott","out","paź","spa","zho","říj","οκτ","жов","кас","окт"},
{"lap","lis","lys","noe","noi","nov","noé","nóv","νοέ","лис","ліс","нов","ное","ноя"},
{"ara","dec","dek","des","det","dez","dic","déc","gru","hru","pro","s’n","δεκ","гру","дек","дец","сьн"}
}
local filename = ''

function ContentGetSupportedField(FieldIndex)
  if fields[FieldIndex + 1] ~= nil then
    return fields[FieldIndex + 1], "", 8
  end
  return "", "", 0
end

function ContentGetDefaultSortOrder(FieldIndex)
  return 1; --or -1
end

function ContentGetDetectString()
  return 'EXT="TXT"'
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
  if FieldIndex >= #fields then return nil end
  if filename ~= FileName then
    local at = SysUtils.FileGetAttr(FileName)
    if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
    local e = string.lower(SysUtils.ExtractFileExt(FileName))
    if e ~= '.txt' then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    local ch, cb = 1, 1
    local rn, re = false, false
    head = {}
    body = {}
    for l in h:lines() do
      l = string.gsub(l, '[\r\n]+$', '')
      if rn == false then
        if l == 'Content-Type: text/x-zim-wiki' then rn = true else break end
      else
        if (l == '') and (re == false) then
          re = true
        else
          if re == true then
            body[cb] = l
            cb = cb + 1
          else
            head[ch] = l
            ch = ch + 1
          end
        end
      end
      if cb == 3 then break end
    end
    h:close()
    if #head == 0 then
      return nil
    else
      if head[1] == nil then return nil end
      if string.sub(head[1], 1, 12) ~= 'Wiki-Format:' then return nil end
    end
    filename = FileName
  end
  local s
  if FieldIndex == 0 then
    return string.gsub(string.sub(head[1], 13, -1), '^[\t ]+', '')
  elseif FieldIndex == 1 then
    for i = 2, #head do
      if string.sub(head[i], 1, 14) == 'Creation-Date:' then
        s = string.gsub(string.sub(head[i], 15, -1), '^[\t ]+', '')
        if s == 'Unknown' then
          s = nil
        else
          s = string.sub(s, 1, 10)
        end
        break
      end
    end
    return s
  elseif FieldIndex == 2 then
    if body[1] == nil then return nil end
    if string.sub(body[1], 1, 1) ~= '=' then return nil end
    s = string.gsub(body[1], '^=+ +', '')
    s = string.gsub(s, ' +=+$', '')
    return s
  elseif FieldIndex == 3 then
    if body[2] == nil then return nil end
    local dt = {}
    dt.day, s, dt.year= string.match(body[2], '^[^ ]+ [^ ]+ (%d+) ([^ ]+) (%d+)$')
    if dt.day == nil then return nil end
    for k, v in pairs(dt) do dt[k] = tonumber(v) end
    dt.month = GetMonthNumber(s)
    if dt.month == nil then return nil end
    dt.hour, dt.min, dt.sec = 0, 0, 1
    return os.date('%Y-%m-%d', os.time(dt))
  elseif (FieldIndex > 3) and (FieldIndex < 8) then
    s = SysUtils.ExtractFileDir(FileName)
    local f = s .. SysUtils.PathDelim .. 'notebook.zim'
    local c = 0
    while true do
      if SysUtils.FileExists(f) then break end
      if c == 1 then
        f = nil
        break
      end
      s = SysUtils.ExtractFileDir(s)
      if string.sub(s, -1, -1) == SysUtils.PathDelim then
        f = s .. 'notebook.zim'
        c = 1
      else
        f = s .. SysUtils.PathDelim .. 'notebook.zim'
      end
    end
    if f ~= nil then
      if FieldIndex == 6 then
        s = SysUtils.ExtractFileName(s)
        if s == '' then return nil else return s end
      elseif FieldIndex == 4 then
        s = SysUtils.ExtractFilePath(f)
        c = string.len(s)
        f = string.sub(FileName, 1, c)
        if s == f then
          s = string.sub(FileName, c + 1, -5)
          return string.gsub(s, SysUtils.PathDelim, ':')
        end
      elseif FieldIndex == 5 then
        return f
      elseif FieldIndex == 7 then
        s = ''
        local h = io.open(f, 'r')
        if h == nil then return nil end
        for l in h:lines() do
          l = string.gsub(l, '[\r\n]+$', '')
          if string.sub(l, 1, 5) == 'name=' then
            s = string.sub(l, 6, -1)
            break
          end
        end
        h:close()
        if s == '' then return nil else return s end
      end
    end
  end
  return nil
end

function GetMonthNumber(s)
  s = LazUtf8.LowerCase(LazUtf8.Copy(s, 1, 8))
  if (s == "červenec") or (s == "července") then return 7 end
  s = LazUtf8.Copy(s, 1, 6)
  if (s == "červen") or (s == "června") then return 6 end
  s = LazUtf8.Copy(s, 1, 4)
  for i = 1, 12 do
    for j = 1, #month1[i] do
      if s == month1[i][j] then return i end
    end
  end
  s = LazUtf8.Copy(s, 1, 3)
  for i = 1, 12 do
    for j = 1, #month2[i] do
      if s == month2[i][j] then return i end
    end
  end
  return nil
end
