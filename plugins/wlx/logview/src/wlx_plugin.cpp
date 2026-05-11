#include "wlxplugin.h"
#include <QWidget>
#include <QDebug>
#include <QTimer>
#include <exception>

#include "LogViewerWidget.h"

// Define a visibility macro for exported functions
#define EXPORT __attribute__((visibility("default")))

extern "C" {

EXPORT HWND DCPCALL ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags) {
    try {
        qDebug() << "ListLoad called for file:" << FileToLoad;
        
        // On Double Commander's Qt6 widgetset, ParentWin is a QWidget* pointer
        // (not an X11 window ID). Simply cast and use as the parent widget.
        QWidget *parentWidget = (QWidget*)ParentWin;
        
        LogViewerWidget *viewer = new LogViewerWidget(parentWidget);

        // Load data immediately so the model and QFileSystemWatcher are
        // set up right away. This also prevents a race where ListLoadNext
        // could be called before a deferred loadFile executes.
        viewer->loadFile(QString::fromUtf8(FileToLoad));

        // FOCUS LAYER 0 — Deferred show().
        // When DC user clicks a file, DC sends MousePress, then immediately
        // calls ListLoad. If we show() synchronously, the new widget can
        // trap DC's MouseRelease event, causing a "phantom-drag" state where
        // DC believes the mouse is still held down.
        // By deferring show() by 50ms, DC finishes its mouse event processing.
        QTimer::singleShot(50, viewer, [viewer]() {
            viewer->show();
        });
        
        return (HWND)viewer;
    } catch (const std::exception& e) {
        qCritical() << "ListLoad exception:" << e.what();
    } catch (...) {
        qCritical() << "ListLoad unknown exception";
    }
    return nullptr;
}

EXPORT int DCPCALL ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags) {
    try {
        qDebug() << "ListLoadNext called for file:" << FileToLoad;
        
        // PluginWin is the QWidget* pointer returned by ListLoad
        LogViewerWidget *viewer = qobject_cast<LogViewerWidget*>((QWidget*)PluginWin);
        if (viewer) {
            viewer->loadFile(QString::fromUtf8(FileToLoad));
            return LISTPLUGIN_OK;
        }
    } catch (const std::exception& e) {
        qCritical() << "ListLoadNext exception:" << e.what();
    } catch (...) {
        qCritical() << "ListLoadNext unknown exception";
    }
    return LISTPLUGIN_ERROR;
}

EXPORT void DCPCALL ListCloseWindow(HWND ListWin) {
    try {
        qDebug() << "ListCloseWindow called";
        // ListWin is the QWidget* pointer returned by ListLoad
        QWidget *widget = (QWidget*)ListWin;
        if (widget) {
            delete widget; // Deterministic destruction
        }
    } catch (const std::exception& e) {
        qCritical() << "ListCloseWindow exception:" << e.what();
    } catch (...) {
        qCritical() << "ListCloseWindow unknown exception";
    }
}

EXPORT int DCPCALL ListSearchText(HWND ListWin, char* SearchString, int SearchParameter) {
    try {
        qDebug() << "ListSearchText called:" << SearchString;
        // ListWin is the QWidget* pointer returned by ListLoad
        LogViewerWidget *viewer = qobject_cast<LogViewerWidget*>((QWidget*)ListWin);
        if (viewer) {
            viewer->triggerSearch(QString::fromUtf8(SearchString), SearchParameter);
            return LISTPLUGIN_OK;
        }
    } catch (const std::exception& e) {
        qCritical() << "ListSearchText exception:" << e.what();
    } catch (...) {
        qCritical() << "ListSearchText unknown exception";
    }
    return LISTPLUGIN_ERROR;
}

} // extern "C"
