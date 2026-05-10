#include "wlx_plugin.h"
#include "editor_widget.h"
#include <QString>
#include <cstring>
#include <QWidget>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QObject>

static void logLoad(const QString &msg) {
    const QString dir = QStringLiteral(RICH_EDITOR_QT_LOG_DIR);
    QDir().mkpath(dir);
    QFile f(dir + QStringLiteral("/rich_editor_qt_wlx.log"));
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;
    const QString line = QStringLiteral("%1 %2\n")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), msg);
    f.write(line.toUtf8());
}

static QHash<void*, EditorWidget*> g_instances;

static void trackParentLifetime(QWidget *parent, void *key) {
    // Ensure we clean up when DC destroys the viewer container.
    QObject::connect(parent, &QObject::destroyed, parent, [key]() {
        EditorWidget *w = g_instances.take(key);
        if (w) {
            logLoad(QStringLiteral("parent destroyed -> delete editor=%1")
                        .arg(reinterpret_cast<qulonglong>(w), 0, 16));
            delete w;
        }
    });
}

HWND ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags) {
    auto* parentWidget = static_cast<QWidget*>(ParentWin);
    const QString path = QString::fromUtf8(FileToLoad);
    logLoad(QStringLiteral("ListLoad parent=%1 path='%2' build='%3'")
                .arg(reinterpret_cast<qulonglong>(ParentWin), 0, 16)
                .arg(path, QStringLiteral(__DATE__ " " __TIME__)));

    EditorWidget *editor = g_instances.value(ParentWin, nullptr);
    if (editor && editor->parentWidget() == parentWidget) {
        // If DC is re-calling ListLoad for the same viewer panel, reuse the widget
        // to avoid losing unsaved edits.
        const QString cur = editor->currentFilePath();
        if (cur == path && editor->isModified()) {
            editor->show();
            return static_cast<HWND>(editor);
        }
        if (editor->loadFile(path)) {
            editor->show();
            return static_cast<HWND>(editor);
        }
        return static_cast<HWND>(editor);
    }

    editor = new EditorWidget(parentWidget);
    g_instances.insert(ParentWin, editor);
    trackParentLifetime(parentWidget, ParentWin);

    if (editor->loadFile(path)) {
        editor->show();
        return static_cast<HWND>(editor);
    }
    
    g_instances.remove(ParentWin);
    delete editor;
    return nullptr;
}

HWND ListLoadW(HWND ParentWin, char16_t* FileToLoad, int ShowFlags) {
    auto* parentWidget = static_cast<QWidget*>(ParentWin);
    const QString path = QString::fromUtf16(FileToLoad);
    logLoad(QStringLiteral("ListLoadW parent=%1 path='%2' build='%3'")
                .arg(reinterpret_cast<qulonglong>(ParentWin), 0, 16)
                .arg(path, QStringLiteral(__DATE__ " " __TIME__)));

    EditorWidget *editor = g_instances.value(ParentWin, nullptr);
    if (editor && editor->parentWidget() == parentWidget) {
        const QString cur = editor->currentFilePath();
        if (cur == path && editor->isModified()) {
            editor->show();
            return static_cast<HWND>(editor);
        }
        if (editor->loadFile(path)) {
            editor->show();
            return static_cast<HWND>(editor);
        }
        return static_cast<HWND>(editor);
    }

    editor = new EditorWidget(parentWidget);
    g_instances.insert(ParentWin, editor);
    trackParentLifetime(parentWidget, ParentWin);

    if (editor->loadFile(path)) {
        editor->show();
        return static_cast<HWND>(editor);
    }
    
    g_instances.remove(ParentWin);
    delete editor;
    return nullptr;
}

void ListCloseWindow(HWND ListWin) {
    auto* editor = static_cast<EditorWidget*>(ListWin);
    if (editor) {
        logLoad(QStringLiteral("ListCloseWindow editor=%1").arg(reinterpret_cast<qulonglong>(ListWin), 0, 16));
        // DC may call ListCloseWindow as part of an internal "reload" cycle.
        // Deleting here loses unsaved edits and causes the "edit -> instant reload" symptom.
        // We keep the widget alive and just hide it; it will be destroyed when ParentWin dies.
        editor->hide();
    }
}

void ListGetDetectString(char* DetectString, int maxlen) {
    const char* detectStr = "EXT=\"TXT\" | EXT=\"PAS\" | EXT=\"C\" | EXT=\"CPP\" | EXT=\"H\" | EXT=\"PY\" | EXT=\"JS\" | EXT=\"HTML\" | EXT=\"CSS\" | EXT=\"JSON\" | EXT=\"XML\" | EXT=\"MD\" | EXT=\"SH\"";
    strncpy(DetectString, detectStr, maxlen - 1);
    DetectString[maxlen - 1] = '\0';
}

int ListSendCommand(HWND ListWin, int Command, int Parameter) {
    auto* editor = static_cast<EditorWidget*>(ListWin);
    if (!editor) return LISTPLUGIN_ERROR;

    logLoad(QStringLiteral("ListSendCommand cmd=%1 param=%2")
                .arg(Command)
                .arg(Parameter));
    
    switch (Command) {
        case lc_copy:
            editor->copySelection();
            return LISTPLUGIN_OK;
        case lc_newparams:
            // DC may send this frequently (e.g. on file change events). Returning ERROR
            // can cause DC to destroy/recreate the plugin, which looks like "reloads".
            // We intentionally accept and ignore it unless we learn we need its payload.
            return LISTPLUGIN_OK;
        case lc_selectall:
            editor->selectAll();
            return LISTPLUGIN_OK;
        case lc_focus:
            // DC uses lc_focus to indicate focus gained/lost.
            // Parameter: 1 = focus gained, 0 = focus lost (common convention).
            editor->hostSetFocus(Parameter != 0);
            return LISTPLUGIN_OK;
    }
    
    return LISTPLUGIN_ERROR;
}
