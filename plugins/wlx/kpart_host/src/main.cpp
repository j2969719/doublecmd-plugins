#include "wlxplugin.h"
#include "kpartwidget.h"
#include <QCoreApplication>
#include <QGuiApplication>
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
    KPartWidget *view = new KPartWidget(parent);
    
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

    QString fileName = QString::fromUtf16(reinterpret_cast<const char16_t*>(FileToLoad));
    
    QWidget *parent = static_cast<QWidget*>(ParentWin);
    KPartWidget *view = new KPartWidget(parent);

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
    
    KPartWidget *view = static_cast<KPartWidget*>(PluginWin);
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
    
    KPartWidget *view = static_cast<KPartWidget*>(PluginWin);
    if (!view) return LISTPLUGIN_ERROR;

    QString fileName = QString::fromUtf16(reinterpret_cast<const char16_t*>(FileToLoad));
    if (view->loadFile(fileName)) {
        return LISTPLUGIN_OK;
    }
    return LISTPLUGIN_ERROR;
}

void DCPCALL ListCloseWindow(HWND ListWin)
{
    KPartWidget *view = static_cast<KPartWidget*>(ListWin);
    if (view) {
        delete view;
    }
}

void DCPCALL ListGetDetectString(char* DetectString, int maxlen)
{
    // Broad detection for formats handled by KParts (Documents, Ebooks, Source Code, Fonts)
    snprintf(DetectString, maxlen, 
        "EXT=\"PDF\" | EXT=\"EPUB\" | EXT=\"CBR\" | EXT=\"CBZ\" | "
        "EXT=\"ODT\" | EXT=\"DOCX\" | EXT=\"XLSX\" | EXT=\"PPTX\" | "
        "EXT=\"MD\" | EXT=\"TXT\" | EXT=\"CPP\" | EXT=\"PY\" | EXT=\"JS\" | "
        "EXT=\"TTF\" | EXT=\"OTF\" | EXT=\"SVG\"");
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
    
    // Set application metadata once during plugin global initialization
    // This helps KDE Frameworks associate jobs with the application.
    if (QCoreApplication::instance()) {
        if (QCoreApplication::applicationName().isEmpty()) {
            QCoreApplication::setApplicationName(QStringLiteral("doublecmd"));
        }
        if (QGuiApplication::desktopFileName().isEmpty()) {
            QGuiApplication::setDesktopFileName(QStringLiteral("doublecmd"));
        }
    }
}

} // extern "C"
