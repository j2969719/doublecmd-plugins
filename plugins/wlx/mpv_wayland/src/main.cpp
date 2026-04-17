#include "wlxplugin.h"
#include "mpvwidget.h"
#include <QCoreApplication>
#include <QWidget>
#include <cstdio>

extern "C" {

HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags)
{
    (void)ShowFlags;

    if (!QCoreApplication::instance()) {
        return nullptr;
    }

    QWidget *parent = static_cast<QWidget*>(ParentWin);
    MpvWidget *view = new MpvWidget(parent);
    
    if (view->loadFile(QString::fromUtf8(FileToLoad))) {
        view->show();
        return static_cast<HWND>(view);
    } else {
        delete view;
        return nullptr;
    }
}

HWND DCPCALL ListLoadW(HWND ParentWin, WCHAR* FileToLoad, int ShowFlags)
{
    (void)ShowFlags;

    if (!QCoreApplication::instance()) {
        return nullptr;
    }

    // QString::fromWCharArray works with wchar_t*, but WCHAR is uint16_t in common.h
    QString fileName = QString::fromUtf16(reinterpret_cast<const char16_t*>(FileToLoad));
    
    QWidget *parent = static_cast<QWidget*>(ParentWin);
    MpvWidget *view = new MpvWidget(parent);
    
    if (view->loadFile(fileName)) {
        view->show();
        return static_cast<HWND>(view);
    } else {
        delete view;
        return nullptr;
    }
}

int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags)
{
    (void)ParentWin;
    (void)ShowFlags;
    
    MpvWidget *view = static_cast<MpvWidget*>(PluginWin);
    if (!view) return LISTPLUGIN_ERROR;

    if (view->loadFile(QString::fromUtf8(FileToLoad))) {
        return LISTPLUGIN_OK;
    }
    return LISTPLUGIN_ERROR;
}

int DCPCALL ListLoadNextW(HWND ParentWin, HWND PluginWin, WCHAR* FileToLoad, int ShowFlags)
{
    (void)ParentWin;
    (void)ShowFlags;
    
    MpvWidget *view = static_cast<MpvWidget*>(PluginWin);
    if (!view) return LISTPLUGIN_ERROR;

    QString fileName = QString::fromUtf16(reinterpret_cast<const char16_t*>(FileToLoad));
    if (view->loadFile(fileName)) {
        return LISTPLUGIN_OK;
    }
    return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
    MpvWidget *view = static_cast<MpvWidget*>(ListWin);
    if (view) {
        view->closeFile();
        delete view;
    }
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
    // Basic detection for video/audio files
    snprintf(DetectString, maxlen, "EXT=\"MKV\" | EXT=\"MP4\" | EXT=\"AVI\" | EXT=\"MOV\" | EXT=\"MP3\" | EXT=\"FLAC\" | EXT=\"WAV\" | EXT=\"OGG\" | EXT=\"WEBM\"");
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
    (void)ListWin;
    (void)FindNext;
    return LISTPLUGIN_OK;
}

int DCPCALL ListSendCommand(HWND ListWin, int Command, int Parameter)
{
    (void)ListWin;
    (void)Command;
    (void)Parameter;
    return LISTPLUGIN_ERROR;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
    (void)dps;
}

} // extern "C"
