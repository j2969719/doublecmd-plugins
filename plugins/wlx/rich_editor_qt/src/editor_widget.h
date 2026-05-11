#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QToolBar>
#include <QLabel>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QFile>
#include <QKeyEvent>
#include <QToolButton>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>

// KDE Frameworks TextEditor headers
#include <KTextEditor/Document>
#include <KTextEditor/View>

class EditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit EditorWidget(QWidget *parent = nullptr);
    ~EditorWidget() override;

    bool loadFile(const QString &filePath);
    
    void copySelection();
    void selectAll();
    void hostSetFocus(bool focus);
    QString currentFilePath() const;
    bool isModified() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    KTextEditor::Document *m_doc;
    KTextEditor::View *m_view;
    
    QVBoxLayout *m_layout;
    QToolBar *m_toolbar;
    QMenuBar *m_menuBar;
    
    // Status Bar items
    QWidget *m_statusBar;
    QHBoxLayout *m_statusLayout;
    QLabel *m_statusPosition;
    QLabel *m_statusEncoding;
    QLabel *m_statusSyntax;
    QLabel *m_statusIndent;
    QLabel *m_statusMode;
    QLabel *m_statusZoom;

    // Actions
    QAction *m_actionReadOnly;
    QAction *m_actionWordWrap;
    QAction *m_actionShowHidden;

    int m_zoomLevel;
    bool m_isRestoringFocus;

    KTextEditor::Cursor m_savedCursor;
    KTextEditor::Range m_savedSelection;
    bool m_savedSelectionValid;

    // Inline "file changed on disk" banner (Kate-style)
    QWidget *m_diskChangeBar;
    QLabel *m_diskChangeLabel;
    QToolButton *m_diskChangeEnableAutoReload;
    QToolButton *m_diskChangeViewDiff;
    QPushButton *m_diskChangeReload;
    QPushButton *m_diskChangeIgnore;

    void setupMenu();
    void setupToolBar();
    void setupStatusBar();
    void updateStatusBar();
    void installFocusGuard();
    void forceDisableAutoReload();
    void enableAutoReload();
    void showDiffAgainstDisk();
    void setHiddenCharactersVisible(bool visible);
    void hideDiskChangeBar();

    KTextEditor::Cursor findWordStart(const KTextEditor::Cursor &cursor) const;
    KTextEditor::Cursor findWordEnd(const KTextEditor::Cursor &cursor) const;
    void saveDocument();

    // Focus handling
    bool m_isActive;
    void setActive(bool active);

private slots:
    void onCursorPositionChanged();
    void onDocumentModeChanged();
    
    void convertToUpper();
    void convertToLower();
    void convertToTitleCase();
    void convertToProperCase();
    void convertToCamelCase();
    void convertToSnailCase();
    
    void convertEolToWin();
    void convertEolToLinux();
    void convertEolToMac();
    
    void sortLines();
    void trimTrailingSpaces();
    void toggleHiddenCharacters();
    void setEncoding(const QString& encoding);
    
    void zoomIn();
    void zoomOut();
    void zoomReset();
};
