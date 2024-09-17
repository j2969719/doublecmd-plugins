Extension API
==========

Double Commander's Extension API, which allows to create platform-independent dialog windows in plugins.

- [Dialog API](dialog_api.md)
- [Translation](translation.md)

Supported [plugin API](https://github.com/doublecmd/doublecmd/wiki/Plugins-development):

- `WCX`
- `WFX`

# Functions

## ExtensionInitialize
Plugin must implement this function for working with Extension API.

```c
void DCPCALL ExtensionInitialize(tExtensionStartupInfo* StartupInfo);
```

### Parameters:

- `StartupInfo` - pointer to ExtensionStartupInfo structure (use memcpy, Luke)

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
  void *Translation;
  tTranslateStringProc TranslateString;
  uintptr_t VersionAPI;
  tMsgChoiceBoxProc MsgChoiceBox;
  tDialogBoxParamProc DialogBoxParam;
  tSetProperty SetProperty;
  tGetProperty GetProperty;
  unsigned char Reserved[4089 * sizeof(void *)];
} tExtensionStartupInfo;
```
