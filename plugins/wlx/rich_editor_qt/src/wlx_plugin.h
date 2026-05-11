#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Double Commander WLX API
typedef void* HWND;

#define LISTPLUGIN_OK 0
#define LISTPLUGIN_ERROR 1

#define lc_copy 1
#define lc_newparams 2
#define lc_selectall 3
#define lc_setpercent 4
#define lc_focus 5

HWND ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags);
HWND ListLoadW(HWND ParentWin, char16_t* FileToLoad, int ShowFlags);
void ListCloseWindow(HWND ListWin);
void ListGetDetectString(char* DetectString, int maxlen);
int ListSendCommand(HWND ListWin, int Command, int Parameter);

#ifdef __cplusplus
}
#endif
