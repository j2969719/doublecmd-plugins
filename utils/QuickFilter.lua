-- QuickFilter.lua (cross-platform)
-- 2021.04.02
--[[
Quick filter with a predefined list of masks (see and use table "alist").
"Specify a mask..." for specify on the fly (will not be saved).
]]

local alist = {
"*.docx;*.doc",
"*.pdf;*.djvu;*.djv",
"*.png",
"*.ape;*.apl;*.flac;*.la;*.tak;*.tta;*.wv;*.wvp",
'   Specify a mask...'
}

local sf = Dialogs.InputListBox('Quick filter', 'Select:', alist, 1)
if sf == nil then return end
if sf == '   Specify a mask...' then
  local ba, sa = Dialogs.InputQuery('Quick filter', 'Mask:', false, '*')
  if ba == false then
    return
  else
    sf = sa
  end
end

DC.ExecuteCommand('cm_QuickFilter', 'filter=on', 'matchbeginning=off', 'matchending=off', 'casesensitive=off', 'files=on', 'directories=on', 'text=' .. sf)
