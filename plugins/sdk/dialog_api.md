Extension (Dialog) API
==========

# Functions

## ExtensionInitialize
Plugin must implement this function for working with Extension API.


```c
void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo);
```

## ExtensionFinalize
Finalize function


```c
void DCPCALL ExtensionFinalize(void* Reserved);
```


# Structures

## tExtensionStartupInfo

`tExtensionStartupInfo` is used in `ExtensionInitialize`.


```c
typedef struct {
	uint32_t StructSize;
	char PluginDir[EXT_MAX_PATH];
	char PluginConfDir[EXT_MAX_PATH];
	tInputBoxProc InputBox;
	tMessageBoxProc MessageBox;
	tDialogBoxLFMProc DialogBoxLFM;
	tDialogBoxLRSProc DialogBoxLRS;
	tDialogBoxLFMFileProc DialogBoxLFMFile;
	tDlgProc SendDlgMsg;
	unsigned char Reserved[4096 * sizeof(void *)];
} tExtensionStartupInfo;
```



## Callbacks

## tMessageBoxProc
MessageBox callback function

```c
typedef int (DCPCALL *tMessageBoxProc)(char* Text, char* Caption, long Flags);
```

### Flags

#### Buttons

- `MB_OK`
- `MB_OKCANCEL`
- `MB_ABORTRETRYIGNORE`
- `MB_YESNOCANCEL`
- `MB_YESNO`
- `MB_RETRYCANCEL`

#### Default button

- `MB_DEFBUTTON1`
- `MB_DEFBUTTON2`
- `MB_DEFBUTTON3`
- `MB_DEFBUTTON4`

#### Icon

- `MB_ICONHAND`
- `MB_ICONQUESTION`
- `MB_ICONEXCLAMATION`
- `MB_ICONASTERICK`
- `MB_ICONWARNING`
- `MB_ICONERROR`
- `MB_ICONSTOP`
- `MB_ICONINFORMATION`

### Return values

- `ID_OK`
- `ID_CANCEL`
- `ID_ABORT`
- `ID_RETRY`
- `ID_IGNORE`
- `ID_YES`
- `ID_NO`
- `ID_CLOSE`
- `ID_HELP`

## tInputBoxProc
InputBox callback function

```c
typedef BOOL (DCPCALL *tInputBoxProc)(char* Caption, char* Prompt, BOOL MaskInput, char* Value, int ValueMaxLen);
```

# Custom LFM dialog's callbacks

## tDlgProc
Dialog window callback function


```c
typedef intptr_t (DCPCALL *tDlgProc)(uintptr_t pDlg, char* DlgItemName, intptr_t Msg, intptr_t wParam, intptr_t lParam);
```

### Events messages (`Msg`)
- `DN_CLICK` - Sent after mouse click
- `DN_DBLCLICK` - Sent after mouse double click
- `DN_CHANGE` - Sent after the dialog item is changed
- `DN_GOTFOCUS` - Sent when the dialog item gets input focus
- `DN_INITDIALOG` - Sent before showing the dialog
- `DN_KILLFOCUS` - Sent before a dialog item loses the input focus
- `DN_KEYDOWN`
- `DN_KEYUP`

???

- `DN_CLOSE` - Sent before the dialog is closed
- `DM_USER` - Starting value for user defined messages

### Dialog messages (`SendDlgMsg`)
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

## tDialogBoxLFMProc
Parse Lazarus Form from data


```c
typedef BOOL (DCPCALL *tDialogBoxLFMProc)(intptr_t LFMData, unsigned long DataSize, tDlgProc DlgProc);
```

## tDialogBoxLRSProc
Parse Lazarus Resource Form from data


```c
typedef BOOL (DCPCALL *tDialogBoxLRSProc)(intptr_t LRSData, unsigned long DataSize, tDlgProc DlgProc);
```

## tDialogBoxLFMFileProc
Parse Lazarus Form from LFM file


```c
typedef BOOL (DCPCALL *tDialogBoxLFMFileProc)(char* LFMFileName, tDlgProc DlgProc);
```


# Supported LCL controls

- [TButton](https://wiki.lazarus.freepascal.org/TButton)
- [TBitBtn](https://wiki.lazarus.freepascal.org/TBitBtn)
- [TFileNameEdit](https://wiki.lazarus.freepascal.org/TFileNameEdit)
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

# Supported events

## Dialog events
- `DialogBoxShow` - mandatory

## Button/BitBtn events
- `ButtonClick`
- `ButtonEnter`
- `ButtonExit`
- `ButtonKeyDown`
- `ButtonKeyUp`

## ComboBox events
- `ComboBoxClick`
- `ComboBoxDblClick`
- `ComboBoxChange`
- `ComboBoxEnter`
- `ComboBoxExit`
- `ComboBoxKeyDown`
- `ComboBoxKeyUp`

## Edit events
- `EditClick`
- `EditDblClick`
- `EditChange`
- `EditEnter`
- `EditExit`
- `EditKeyDown`
- `EditKeyUp`

## ListBox events
- `ListBoxClick`
- `ListBoxDblClick`
- `ListBoxChange`
- `ListBoxEnter`
- `ListBoxExit`
- `ListBoxKeyDown`
- `ListBoxKeyUp`

## CheckBox events
- `CheckBoxChange`
