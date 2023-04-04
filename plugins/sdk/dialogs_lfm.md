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
- `DM_CLOSE` - A signal that the dialog is about to close
- `DM_ENABLE`
- `DM_GETDLGDATA`
- `DM_GETDLGBOUNDS`
- `DM_GETITEMBOUNDS`
- `DM_GETTEXT` - Retrieve the text of an edit string or the caption of an item
- `DM_KEYDOWN`
- `DM_KEYUP`
- `DM_SETDLGDATA`
- `DM_SETFOCUS` - Set the keyboard focus to the given dialog item
- `DM_REDRAW` - Redraw the whole dialog
- `DM_SETTEXT` - Set a new string value for an edit line or a new caption for an item
- `DM_SETMAXTEXTLENGTH` - Set the maximum length of an edit string
- `DM_SHOWDIALOG` - Show/hide the dialog window
- `DM_SHOWITEM` - Show/hide a dialog item
- `DM_GETCHECK` - Retrieve the state of TCheckBox or TRadioButton items
- `DM_SETCHECK` - Change the state of TCheckBox and TRadioButton items
- `DM_LISTGETITEM` - Retrieve a list item
- `DM_LISTGETITEMINDEX` - Get current item index in a list
- `DM_LISTSETITEMINDEX` - Set current item index in a list
- `DM_LISTDELETE`
- `DM_LISTADD`
- `DM_LISTADDSTR`
- `DM_LISTUPDATE`
- `DM_LISTINSERT`
- `DM_LISTINDEXOF`
- `DM_LISTGETCOUNT`
- `DM_LISTGETDATA`
- `DM_LISTSETDATA`
- `DM_SETDLGBOUNDS`
- `DM_SETITEMBOUNDS`
- `DM_GETDROPPEDDOWN`
- `DM_SETDROPPEDDOWN`
- `DM_GETITEMDATA`
- `DM_SETITEMDATA`
- `DM_LISTSET`
- `DM_SETPROGRESSVALUE`
- `DM_SETPROGRESSSTYLE`
- `DM_SETPASSWORDCHAR`
- `DM_LISTCLEAR`
- `DM_TIMERSETINTERVAL`

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
|Event|Value|
|---|---|
|`OnShow`|`DialogBoxShow`|
|`OnClose`|`DialogBoxClose`|

## Button/BitBtn events
|Event|Value|
|---|---|
|`OnClick`|`ButtonClick`|
|`OnEnter`|`ButtonEnter`|
|`OnExit`|`ButtonExit`|
|`OnKeyDown`|`ButtonKeyDown`|
|`OnKeyUp`|`ButtonKeyUp`|

## ComboBox events
|Event|Value|
|---|---|
|`OnClick`|`ComboBoxClick`|
|`OnDblClick`|`ComboBoxDblClick`|
|`OnChange`|`ComboBoxChange`|
|`OnEnter`|`ComboBoxEnter`|
|`OnExit`|`ComboBoxExit`|
|`OnKeyDown`|`ComboBoxKeyDown`|
|`OnKeyUp`|`ComboBoxKeyUp`|

## Edit events
|Event|Value|
|---|---|
|`OnClick`|`EditClick`|
|`OnDblClick`|`EditDblClick`|
|`OnChange`|`EditChange`|
|`OnEnter`|`EditEnter`|
|`OnExit`|`EditExit`|
|`OnKeyDown`|`EditKeyDown`|
|`OnKeyUp`|`EditKeyUp`|

## ListBox events
|Event|Value|
|---|---|
|`OnClick`|`ListBoxClick`|
|`OnDblClick`|`ListBoxDblClick`|
|`OnSelectionChange`|`ListBoxSelectionChange`|
|`OnEnter`|`ListBoxEnter`|
|`OnExit`|`ListBoxExit`|
|`OnKeyDown`|`ListBoxKeyDown`|
|`OnKeyUp`|`ListBoxKeyUp`|

## CheckBox events
|Event|Value|
|---|---|
|`OnChange`|`CheckBoxChange`|

## Timer events
|Event|Value|
|---|---|
|`OnTimer`|`TimerTimer`|