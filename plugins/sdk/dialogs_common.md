Common Dialogs
==========

# StartupInfo->MessageBox
MessageBox function
```c
int MessageBox(char* Text, char* Caption, long Flags);
```
## Parameters:
- `Text` - the message to be displayed
- `Caption` - the dialog box title
- `Flags` - the contents and behavior of the dialogbxo

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

## Return value:
Modal result
- `ID_OK`
- `ID_CANCEL`
- `ID_ABORT`
- `ID_RETRY`
- `ID_IGNORE`
- `ID_YES`
- `ID_NO`
- `ID_CLOSE`
- `ID_HELP`

# StartupInfo->InputBox
InputBox function

```c
BOOL InputBox(char* Caption, char* Prompt, BOOL MaskInput, char* Value, int ValueMaxLen);
```
## Parameters:
- `Caption` - the dialog box title
- `Prompt` - the message to be displayed
- `MaskInput` - mask input `*`
- `Value` - output buffer
- `ValueMaxLen` - output buffer length
## Return value:
Modal result

# StartupInfo->MsgChoiceBox
MsgChoiceBox function

```c
int MsgChoiceBox(char* Text, char* Caption, char** Buttons, int BtnDef, int BtnEsc);
```
## Parameters:
- `Text` - the message to be displayed
- `Caption` - the dialog box title
- `Buttons` - array of button names
- `BtnDef` - default button index or -1
- `BtnEsc` - button index trigerred on Esc or -1
## Return value:
Button index
