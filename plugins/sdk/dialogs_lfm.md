Custom Dialogs
==========

# tDlgProc
Dialog window callback function
```c
typedef intptr_t (DCPCALL *tDlgProc)(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam);
```
## Parameters:
- `pDlg` - pointer to dialog
- `DlgItemName` - sender (LCL control) name
- `Msg` - events message
- `wParam` - pointer to additional message-specific data
- `lParam` - pointer to additional message-specific data

### Events messages (`Msg`)
- `DN_CLICK` - Sent after mouse click
- `DN_DBLCLICK` - Sent after mouse double click
- `DN_CHANGE` - Sent after the dialog item is changed
- `DN_GOTFOCUS` - Sent when the dialog item gets input focus
- `DN_INITDIALOG` - Sent before showing the dialog
- `DN_KILLFOCUS` - Sent before a dialog item loses the input focus
- `DN_KEYDOWN`
- `DN_KEYUP`
- `DN_TIMER` - Sent when a timer expires
- `DN_CLOSE` - Sent before the dialog is closed

???

- `DM_USER` - Starting value for user defined messages

## Return value:
Pointer to message-specific result

# StartupInfo->SendDlgMsg
Send Message to dialog
```c
intptr_t SendDlgMsg(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam);
```
## Parameters:
- `pDlg` - pointer to dialog
- `DlgItemName` - LCL control name
- `Msg` - events message
- `wParam` - pointer to additional message-specific data
- `lParam` - pointer to additional message-specific data

### Dialog messages
|Message|Component|wParam|lParam|Result|Comment|
|---|---|---|---|---|---|
|`DM_CLOSE`|DialogBox|`intptr_t` ModalResult|-|-|A signal that the dialog is about to close|
|`DM_ENABLE`|*|`bool` Enabled|-|`bool` PrevEnabled||
|`DM_GETCHECK`|`TCheckBox`, `TRadioButton`|-|-|`intptr_t` State|Retrieve the state of TCheckBox or TRadioButton items|
|`DM_GETDLGBOUNDS`|DialogBox|-|-|`RECT *` Result||
|`DM_GETDLGDATA`|DialogBox|-|-|`intptr_t` Tag||
|`DM_GETDROPPEDDOWN`|`TComboBox`|-|-|`bool` DroppedDown||
|`DM_GETITEMBOUNDS`|*|-|-|`RECT *` Result||
|`DM_GETITEMDATA`|*|-|-|`intptr_t` Tag||
|`DM_LISTADD`|`TComboBox`, `TListBox`, `TMemo`|`char *` Text|`intptr_t` UserData|`intptr_t` Result||
|`DM_LISTADDSTR`|`TComboBox`, `TListBox`, `TMemo`|`char *` Text|-|`intptr_t` Result||
|`DM_LISTDELETE`|`TComboBox`, `TListBox`, `TMemo`|`intptr_t` Index|-|-||
|`DM_LISTINDEXOF`|`TComboBox`, `TListBox`, `TMemo`|-|`char *` Text|`intptr_t` Result||
|`DM_LISTINSERT`|`TComboBox`, `TListBox`, `TMemo`|`intptr_t` Index|`char *` Text|-||
|`DM_LISTGETCOUNT`|`TComboBox`, `TListBox`, `TMemo`|-|-|`intptr_t` Count||
|`DM_LISTGETDATA`|`TComboBox`, `TListBox`, `TMemo`|`intptr_t` Index|-|`intptr_t` UserData||
|`DM_LISTGETITEM`|`TComboBox`, `TListBox`, `TMemo`|`intptr_t` Index|-|`char *` Text|Retrieve a list item|
|`DM_LISTGETITEMINDEX`|`TComboBox`, `TListBox`, `TRadioGroup`|-|-|`intptr_t` Index|Get current item index in a list|
|`DM_LISTSETITEMINDEX`|`TComboBox`, `TListBox`, `TRadioGroup`|`intptr_t` Index|-|-|Set current item index in a list|
|`DM_LISTUPDATE`|`TComboBox`, `TListBox`, `TMemo`|`intptr_t` Index|`char *` Text|-||
|`DM_LISTCLEAR`|`TComboBox`, `TListBox`, `TMemo`|-|-|-||
|`DM_GETTEXT`|*|-|-|`char *` Text|Retrieve the text of an edit string or the caption of an item|
|`DM_KEYDOWN`|*|`intptr_t` Key|-|`intptr_t` Key||
|`DM_KEYUP`|*|`intptr_t` Key|-|`intptr_t` Key||
|`DM_REDRAW`|DialogBox|-|-|-|Redraw the whole dialog|
|`DM_SETCHECK`|`TCheckBox`, `TRadioButton`|`bool` State|-|`intptr_t` PrevState|Change the state of TCheckBox and TRadioButton items|
|`DM_LISTSETDATA`|`TComboBox`, `TListBox`, `TMemo`|`intptr_t` Index|`intptr_t` UserData|-||
|`DM_SETDLGBOUNDS`|DialogBox|`RECT *` Rect|-|-||
|`DM_SETDLGDATA`|DialogBox|`intptr_t` Tag|-|`intptr_t` PrevTag||
|`DM_SETDROPPEDDOWN`|`TComboBox`|`bool` DroppedDown|-|-||
|`DM_SETFOCUS`|*|-|-|-|Set the keyboard focus to the given dialog item|
|`DM_SETITEMBOUNDS`|*|`RECT *` Rect|-|-||
|`DM_SETITEMDATA`|*|`intptr_t` Tag|-|-||
|`DM_SETMAXTEXTLENGTH`|`TComboBox`, `TEdit`|`intptr_t` MaxLength|-|`intptr_t` PrevMaxLength|Set the maximum length of an edit string|
|`DM_SETTEXT`|*|`char *` Text|-|-|Set a new string value for an edit line or a new caption for an item|
|`DM_SHOWDIALOG`|DialogBox|`0` - Hide, `1` - Show|-|-|Show/hide the dialog window|
|`DM_SHOWITEM`|*|`bool` Visible|-|`bool` PrevVisible|Show/hide a dialog item|
|`DM_SETPROGRESSVALUE`|`TProgressBar`|`intptr_t` Position|-|-||
|`DM_SETPROGRESSSTYLE`|`TProgressBar`|`intptr_t` Style|-|-|`0` - Normal, `1` - Marquee|
|`DM_SETPASSWORDCHAR`|`TCustomEdit`|`char` PasswordChar|-|-||
|`DM_TIMERSETINTERVAL`|`TTimer`|`intptr_t` Interval|-|-||

## Return value:
Pointer to message-specific result

# StartupInfo->DialogBoxLFM
Parse Lazarus Form from data
```c
BOOL DialogBoxLFM(intptr_t LFMData, unsigned long DataSize, tDlgProc DlgProc);
```
## Parameters:
- `LFMData` - pointer to LFM data
- `DataSize` - size of data
- `DlgProc` - dialog window callback function

## Return value:
Modal result

# StartupInfo->DialogBoxLRS
Parse Lazarus Resource Form from data
```c
BOOL DialogBoxLRS(intptr_t LRSData, unsigned long DataSize, tDlgProc DlgProc);
```
## Parameters:
- `LRSData` - pointer to LRS data
- `DataSize` - size of data
- `DlgProc` - dialog window callback function

## Return value:
Modal result

# StartupInfo->DialogBoxLFMFile
Parse Lazarus Form from LFM file
```c
BOOL DialogBoxLFMFile(char* LFMFileName, tDlgProc DlgProc);
```
## Parameters:
- `LFMFileName` - path to LFM file
- `DlgProc` - dialog window callback function

## Return value:
Modal result

# StartupInfo->DialogBoxParam
Parse Lazarus Form from LFM file
```c
BOOL DialogBoxParam(void* Data, uint32_t DataSize, tDlgProc DlgProc, uint32_t Flags, void *UserData, void* Reserved);
```
## Parameters:
- `Data` - pointer to data (LFM or LRS or LFMFileName)
- `DataSize` - size of data
- `DlgProc` - dialog window callback function
- `Flags` - `Data` form
- `UserData` - pointer to userdata
- `Reserved` - ...

#### Flags
- `DB_LFM` - `Data` contains a form in the LFM format
- `DB_LRS` - `Data` contains a form in the LRS format
- `DB_FILENAME` - `Data`  contains a form file name (*.lfm)

## Return value:
Modal result

# Supported LCL controls

- [TTimer](https://wiki.lazarus.freepascal.org/TTimer)
- [TButton](https://wiki.lazarus.freepascal.org/TButton)
- [TBitBtn](https://wiki.lazarus.freepascal.org/TBitBtn)
- [TFileNameEdit](https://wiki.lazarus.freepascal.org/TFileNameEdit)
- [TDirectoryEdit](https://wiki.lazarus.freepascal.org/TDirectoryEdit)
- [TComboBox](https://wiki.lazarus.freepascal.org/TComboBox)
- [TListBox](https://wiki.lazarus.freepascal.org/TListBox)
- [TCheckBox](https://wiki.lazarus.freepascal.org/TCheckBox)
- [TGroupBox](https://wiki.lazarus.freepascal.org/TGroupBox)
- [TLabel](https://wiki.lazarus.freepascal.org/TLabel)
- [TPanel](https://wiki.lazarus.freepascal.org/TPanel)
- [TEdit](https://wiki.lazarus.freepascal.org/TEdit)
- [TMemo](https://wiki.lazarus.freepascal.org/TMemo)
- [TImage](https://wiki.lazarus.freepascal.org/TImage)
- [TTabSheet](https://wiki.lazarus.freepascal.org/TTabSheet)
- [TScrollBox](https://wiki.lazarus.freepascal.org/TScrollBox)
- [TRadioGroup](https://wiki.lazarus.freepascal.org/TRadioGroup)
- [TPageControl](https://wiki.lazarus.freepascal.org/TPageControl)
- [TProgressBar](https://wiki.lazarus.freepascal.org/TProgressBar)
- [TDividerBevel](https://wiki.lazarus.freepascal.org/TDividerBevel)


# Supported events (Lazarus->Object Inspector->Events)

## Dialog events
|Event|Value|Message|wParam|lParam|Comment|
|---|---|---|---|---|---|
|`OnShow`|`DialogBoxShow`|`DN_INITDIALOG`|-|-|Sent before showing the dialog|
|`OnClose`|`DialogBoxClose`|`DN_CLOSE`|-|-|Sent before the dialog is closed|

## Button/BitBtn events
|Event|Value|Message|wParam|lParam|Comment|
|---|---|---|---|---|---|
|`OnClick`|`ButtonClick`|`DN_CLICK`|-|-|Sent after mouse click|
|`OnEnter`|`ButtonEnter`|`DN_GOTFOCUS`|-|-|Sent when the dialog item gets input focus|
|`OnExit`|`ButtonExit`|`DN_KILLFOCUS`|-|-|Sent before a dialog item loses the input focus|
|`OnKeyDown`|`ButtonKeyDown`|`DN_KEYDOWN`|`uint16_t *` Key|`intptr_t` ShiftState|`*Key = 0;` - disable processing in subsequent elements|
|`OnKeyUp`|`ButtonKeyUp`|`DN_KEYUP`|`uint16_t *` Key|`intptr_t` ShiftState|`*Key = 0;` - disable processing in subsequent elements|

## ComboBox events
|Event|Value|Message|wParam|lParam|Comment|
|---|---|---|---|---|---|
|`OnClick`|`ComboBoxClick`|`DN_CLICK`|`intptr_t` ItemIndex|-|Sent after mouse click|
|`OnDblClick`|`ComboBoxDblClick`|`DN_DBLCLICK`|`intptr_t` ItemIndex|-|Sent after mouse double click|
|`OnChange`|`ComboBoxChange`|`DN_CHANGE`|`intptr_t` ItemIndex|-|Sent after the dialog item is changed|
|`OnEnter`|`ComboBoxEnter`|`DN_GOTFOCUS`|-|-|Sent when the dialog item gets input focus|
|`OnExit`|`ComboBoxExit`|`DN_KILLFOCUS`|-|-|Sent before a dialog item loses the input focus|
|`OnKeyDown`|`ComboBoxKeyDown`|`DN_KEYDOWN`|`uint16_t *` Key|`intptr_t` ShiftState|`*Key = 0;` - disable processing in subsequent elements|
|`OnKeyUp`|`ComboBoxKeyUp`|`DN_KEYUP`|`uint16_t *` Key|`intptr_t` ShiftState|`*Key = 0;` - disable processing in subsequent elements|

## Edit events
|Event|Value|Message|wParam|lParam|Comment|
|---|---|---|---|---|---|
|`OnClick`|`EditClick`|`DN_CLICK`|-|-|Sent after mouse click|
|`OnDblClick`|`EditDblClick`|`DN_DBLCLICK`|-|-|Sent after mouse double click|
|`OnChange`|`EditChange`|`DN_CHANGE`|`char *` Text|-|Sent after the dialog item is changed|
|`OnEnter`|`EditEnter`|`DN_GOTFOCUS`|-|-|Sent when the dialog item gets input focus|
|`OnExit`|`EditExit`|`DN_KILLFOCUS`|-|-|Sent before a dialog item loses the input focus|
|`OnKeyDown`|`EditKeyDown`|`DN_KEYDOWN`|`uint16_t *` Key|`intptr_t` ShiftState|`*Key = 0;` - disable processing in subsequent elements|
|`OnKeyUp`|`EditKeyUp`|`DN_KEYUP`|`uint16_t *` Key|`intptr_t` ShiftState|`*Key = 0;` - disable processing in subsequent elements|

## ListBox events
|Event|Value|Message|wParam|lParam|Comment|
|---|---|---|---|---|---|
|`OnClick`|`ListBoxClick`|`DN_CLICK`|`intptr_t` ItemIndex|-|Sent after mouse click|
|`OnDblClick`|`ListBoxDblClick`|`DN_DBLCLICK`|`intptr_t` ItemIndex|-|Sent after mouse double click|
|`OnSelectionChange`|`ListBoxSelectionChange`|`DN_CHANGE`|`intptr_t` ItemIndex|-|Sent after the dialog item is changed|
|`OnEnter`|`ListBoxEnter`|`DN_GOTFOCUS`|-|-|Sent when the dialog item gets input focus|
|`OnExit`|`ListBoxExit`|`DN_KILLFOCUS`|-|-|Sent before a dialog item loses the input focus|
|`OnKeyDown`|`ListBoxKeyDown`|`DN_KEYDOWN`|`uint16_t *` Key|`intptr_t` ShiftState|`*Key = 0;` - disable processing in subsequent elements|
|`OnKeyUp`|`ListBoxKeyUp`|`DN_KEYUP`|`uint16_t *` Key|`intptr_t` ShiftState|`*Key = 0;` - disable processing in subsequent elements|

## CheckBox events
|Event|Value|Message|wParam|lParam|Comment|
|---|---|---|---|---|---|
|`OnChange`|`CheckBoxChange`|`DN_CHANGE`|`bool` Checked|-|Sent after the dialog item is changed|

## Timer events
|Event|Value|Message|wParam|lParam|Comment|
|---|---|---|---|---|---|
|`OnTimer`|`TimerTimer`|`DN_TIMER`|-|-|Sent when a timer expires|