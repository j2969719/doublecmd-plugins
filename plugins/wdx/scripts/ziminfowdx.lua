-- ziminfowdx.lua (cross-platform)
-- 2020.03.22
--[[
Save as UTF-8 without BOM!

Get some info from Zim file (https://zim-wiki.org/)
Supported fields: see table "fields".

Field "Creation date" is optional, from second line of the body:
  "Created [day_of_the_week] [day_of_the_month] [full_month_name] [year]"
  List of month (http://www.webteka.com/months-in-many-languages/):
  Afrikaans, Armenian, Belarusian, Bulgarian, Czech, Danish, Dutch, English, Estonian, Finnish, French,
  German, Greek (and modern), Hungarian, Icelandic, Indonesian, Italian, Latvian, Lithuanian, Norwegian,
  Polish, Portuguese, Romanian, Russian, Serbian, Slovak, Slovenian, Spanish, Swedish, Turkish, Ukrainian
]]

local fields = {
"Header: Wiki-Format",
"Header: Creation-Date",
"Title",
"Creation date",
"Note file",
"Note name"
}
local head = {}
local body = {}
local month = {
"|enero|gennaio|hounvar|ianouários|ianuari|ianuarie|jaanuar|janeiro|januar|januari|januarie|january|január|janvier|janvāris|janúar|leden|ocak|sausis|sichen’|studzien’|styczeń|tammikuu|yanvar’|ιανουάριοσ|студзень|січень|январь |януари|јануар|",
"|febbraio|febrero|febrouários|februar|februari|februarie|february|február|februāris|febrúar|fevereiro|fevral’|fevruari|février|helmikuu|luty|lyuty|lyutyi|pedrvar|vasaris|veebruar|únor|şubat|φεβρουάριοσ|лютий|люты|фебруар|февраль|февруари|",
"|berezen’|březen|kovas|maaliskuu|maart|march|mard|marec|maret|mars|mart|martie|marts|marzec|marzo|março|március|mártios|märts|märz|sakavik|μάρτιοσ|березень|мар|март|сакавік|",
"|abril|aprel’|april|aprile|aprilie|aprill|apríl|aprílios|aprīlis|avril|balandis|duben|huhtikuu|krasavik|kviten|kwiecień|nisan|április|απρίλιοσ|апрель|април|квітень|красавік|",
"|gegužė|květen|maggio|mai|maijs|maio|maj|may|mayis|mayo|mayıs|maí|mei|máios|máj|május|toukokuu|traven’|travien’|μάιοσ|май|мај|травень|",
"|birželis|cherven’|chervien’|czerwiec|giugno|haziran|hoonis|ioúnios|iunie|iyun’|juin|june|junho|juni|junie|junij|junio|juuni|jún|június|júní|jūnijs|kesäkuu|červen|ιούνιοσ|июнь|червень|чэрвень|юни|јуни|",
"|heinäkuu|hoolis|ioúlios|iulie|iyul’|juillet|julho|juli|julie|julij|julio|july|juuli|júl|július|júlí|jūlijs|liepa|lipiec|lipien’|luglio|lypen’|temmuz|červenec|ιούλιοσ|июль|липень|ліпень|юли|јули|",
"|agosto|agustus|août|august|augusti|augusts|augustus|augusztus|avgust|aúgoustos|ağustos|elokuu|ocosdos|rugpjūtis|serpen’|sierpień|srpen|zhnivien’|ágúst|αύγουστοσ|авг|август|жнівень|серпень|",
"|eylül|rusėjis|sebdemper|sentyabr’|september|septembre|septembrie|septembris|septemvri|septiembre|septémbrios|setembro|settembre|syyskuu|szeptember|veresen’|vierasien’|wrzesień|září|σεπτέμβριοσ|верасень|вересень|сентябрь|септембар|септември|",
"|ekim|hokdemper|kastrychnik|lokakuu|october|octobre|octombrie|octubre|octyabr’|oktober|oktobris|oktomvri|oktoober|október|oktṓbrios|ottobre|outubro|październik|spalis|zhovten’|říjen|οκτώβριοσ|жовтень|кастрычнік|октобар|октомври|октябрь|",
"|kasım|lapkritis|listapad|listopad|lystopad|marraskuu|noemvri|noiembrie|november|novembre|novembris|novembro|noviembre|noyabr’|noyemper|noémbrios|nóvember|νοέμβριοσ|листопад|лістапад|новембар|ноември|ноябрь|",
"|aralık|december|decembrie|decembris|dekabr’|dekemvri|dekémbrios|desember|detsember|dezember|dezembro|dicembre|diciembre|décembre|grudzień|gruodis|hruden’|joulukuu|prosinec|s’niezhan’|tegdemper|δεκέμβριοσ|грудень|декабрь|декември|децембар|сьнежань|"
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
  local at = SysUtils.FileGetAttr(FileName)
  if (at < 0) or (math.floor(at / 0x00000010) % 2 ~= 0) then return nil end
  local e
  if filename ~= FileName then
    e = string.lower(SysUtils.ExtractFileExt(FileName))
    if e ~= '.txt' then return nil end
    local h = io.open(FileName, 'r')
    if h == nil then return nil end
    local ch, cb = 1, 1
    local rn, re = false, false
    head = {}
    body = {}
    for l in h:lines() do
      l = string.gsub(l, '\r+$', '')
      if rn == false then
        if l == 'Content-Type: text/x-zim-wiki' then rn = true else break end
      else
        if l == '' then
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
  if FieldIndex == 0 then
    return string.gsub(string.sub(head[1], 13, -1), '^[\t ]+', '')
  elseif FieldIndex == 1 then
    local s
    for i = 2, #head do
      if string.sub(head[i], 1, 14) == 'Creation-Date:' then
        s = string.gsub(string.sub(head[i], 15, -1), '^[\t ]+', '')
        if s == 'Unknown' then s = nil else s = string.gsub(s, 'T', ' ', 1) end
        break
      end
    end
    return s
  elseif FieldIndex == 2 then
    if body[1] == nil then return nil end
    if string.sub(body[1], 1, 1) ~= '=' then return nil end
    local s = string.gsub(body[1], '^=+ +', '')
    s = string.gsub(s, ' +=+$', '')
    return s
  elseif FieldIndex == 3 then
    if body[2] == nil then return nil end
    local dt = {}
    dt.day, dt.month, dt.year= string.match(body[2], '[^ ]+ [^ ]+ (%d+) ([^ ]+) (%d+)')
    if dt.day == nil then return nil end
    dt.month = LazUtf8.LowerCase(dt.month)
    for i = 1, #month do
      if string.find(month[i], '|' .. dt.month, 1, true) ~= nil then
        dt.month = i
        break
      end
    end
    if dt.month <= #month then
      dt.hour, dt.min, dt.sec = 0, 0, 1
      for k, v in pairs(dt) do dt[k] = tonumber(v) end
      return os.date('%Y-%m-%d', os.time(dt))
    end
  elseif (FieldIndex == 4) or (FieldIndex == 5) then
    local s = SysUtils.ExtractFileDir(FileName)
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
      if FieldIndex == 4 then return f end
      local h = io.open(f, 'r')
      if h == nil then return nil end
      local s
      for l in h:lines() do
        l = string.gsub(l, '\r+$', '')
        if string.sub(l, 1, 5) == 'home=' then
          s = string.sub(l, 6, -1)
          break
        end
      end
      h:close()
      return s
    end
  end
  return nil
end
