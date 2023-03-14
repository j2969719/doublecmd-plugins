Translation
==========

## StartupInfo->Translation
Pointer to translation object

```c
void *Translation;
```

## StartupInfo->TranslateString
Get string translation from po-file
```c
int TranslateString(void *Translation, const char *Identifier, const char *Original, char *Output, int OutLen);
```
### Parameters:
- `Translation` - pointer to translation object
- `Identifier` - (e.g. #: tdialogbox.caption)
- `Original` - (msgid)
- `Output` - output buffer
- `OutLen` - output buffer length
### Return value:
String lenght