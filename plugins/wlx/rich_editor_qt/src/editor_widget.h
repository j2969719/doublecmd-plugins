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

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

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

    int m_zoomLevel;

    void setupMenu();
    void setupToolBar();
    void setupStatusBar();
    void updateStatusBar();
    void installFocusGuard();

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
    
    void sortLines();
    void trimTrailingSpaces();
    void toggleHiddenCharacters();
    void setEncoding(const QString& encoding);
    
    void zoomIn();
    void zoomOut();
    void zoomReset();
};
