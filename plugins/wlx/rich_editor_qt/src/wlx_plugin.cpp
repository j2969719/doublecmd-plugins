#include "wlx_plugin.h"
#include "editor_widget.h"
#include <QString>
#include <cstring>
#include <QWidget>

HWND ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags) {
    auto* parentWidget = static_cast<QWidget*>(ParentWin);
    auto* editor = new EditorWidget(parentWidget);
    
    if (editor->loadFile(QString::fromUtf8(FileToLoad))) {
        editor->show();
        return static_cast<HWND>(editor);
    }
    
    delete editor;
    return nullptr;
}

HWND ListLoadW(HWND ParentWin, char16_t* FileToLoad, int ShowFlags) {
    auto* parentWidget = static_cast<QWidget*>(ParentWin);
    auto* editor = new EditorWidget(parentWidget);
    
    if (editor->loadFile(QString::fromUtf16(FileToLoad))) {
        editor->show();
        return static_cast<HWND>(editor);
    }
    
    delete editor;
    return nullptr;
}

void ListCloseWindow(HWND ListWin) {
    auto* editor = static_cast<EditorWidget*>(ListWin);
    if (editor) {
        delete editor;
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
    
    switch (Command) {
        case lc_copy:
            editor->copySelection();
            return LISTPLUGIN_OK;
        case lc_selectall:
            editor->selectAll();
            return LISTPLUGIN_OK;
        case lc_focus:
            editor->setFocus();
            return LISTPLUGIN_OK;
    }
    
    return LISTPLUGIN_ERROR;
}
