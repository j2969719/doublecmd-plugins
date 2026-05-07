#include "editor_widget.h"

#include <KTextEditor/Editor>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <QIcon>
#include <QUrl>
#include <QActionGroup>
#include <QApplication>
#include <QFont>
#include <QStringList>
#include <QMouseEvent>
#include <KSharedConfig>
#include <KConfigGroup>

// Helper: KTextEditor actions live in two places:
//   1. KXMLGUIClient action collection (edit_undo, edit_copy, edit_find, etc.)
//      — accessed via view->action(name)
//   2. Internal KateView children (view_show_whitespaces, tools_toggle_write_lock, etc.)
//      — only accessible via view->findChild<QAction*>(name)
// Try both; the KXMLGUI collection first since it's the standard lookup path.
static QAction* kteAction(KTextEditor::View *view, const char *name) {
    if (!view) return nullptr;
    QAction *a = view->action(name);
    if (a) return a;
    return view->findChild<QAction*>(QString::fromLatin1(name));
}

EditorWidget::EditorWidget(QWidget *parent)
    : QWidget(parent), m_doc(nullptr), m_view(nullptr), m_zoomLevel(10)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Initialize KTextEditor components
    KTextEditor::Editor *editorInfo = KTextEditor::Editor::instance();
    if (!editorInfo) {
        auto* label = new QLabel("Failed to load KTextEditor component.", this);
        m_layout->addWidget(label);
        return;
    }

    m_doc = editorInfo->createDocument(this);
    m_view = m_doc->createView(this);
    m_zoomLevel = m_view->font().pointSize();
    
    // Create UI Elements
    m_menuBar = new QMenuBar(this);
    m_toolbar = new QToolBar(this);
    setupStatusBar();

    // Setup Actions and UI
    setupMenu();
    setupToolBar();

    // Layout Assembly
    m_layout->addWidget(m_menuBar);
    m_layout->addWidget(m_toolbar);
    m_layout->addWidget(m_view, 1); // Expandable
    m_layout->addWidget(m_statusBar);

    m_view->installEventFilter(this);
    if (m_view->focusProxy()) {
        m_view->focusProxy()->installEventFilter(this);
    }
    
    // Strip WA_NativeWindow so KTE doesn't create a Wayland subsurface
    // that holds compositor-level focus independently of the host window
    m_view->setAttribute(Qt::WA_NativeWindow, false);
    if (m_view->focusProxy())
        m_view->focusProxy()->setAttribute(Qt::WA_NativeWindow, false);
    
    // --- Disable ALL auto-reload paths ---
    // 1. Public config key (short-circuits the main auto-reload branch)
    m_doc->setConfigValue("auto-reload-on-external-changes", false);
    // 2. setModifiedOnDiskWarning enables the Kate notification bar instead of silent reload
    m_doc->setModifiedOnDiskWarning(true);
    // 3. AutoReloadIfStateIsInVersionControl has no public alias — write to katepartrc directly
    //    (this is the branch that fires for files in git repos)
    {
        KSharedConfigPtr cfg = KSharedConfig::openConfig(QStringLiteral("katepartrc"));
        KConfigGroup docGrp(cfg, QStringLiteral("Kate Document Defaults"));
        docGrp.writeEntry("Auto Reload If State Is In Version Control", false);
        cfg->sync();
    }
    if (QAction* a = kteAction(m_view, "view_auto_reload")) a->setChecked(false);
    
    // --- Trailing-space stripping: OFF ---
    m_doc->setConfigValue("remove-spaces", 0);
    
    connect(m_view, &KTextEditor::View::cursorPositionChanged, this, &EditorWidget::onCursorPositionChanged);
    connect(m_doc, &KTextEditor::Document::readWriteChanged, this, &EditorWidget::onDocumentModeChanged);
    connect(m_doc, &KTextEditor::Document::modeChanged, this, &EditorWidget::onDocumentModeChanged);
    
    // Re-enforce auto-reload=off whenever KTE fires modifiedOnDisk
    connect(m_doc, &KTextEditor::Document::modifiedOnDisk, this,
        [this](KTextEditor::Document*, bool, KTextEditor::Document::ModifiedOnDiskReason) {
            m_doc->setConfigValue("auto-reload-on-external-changes", false);
            m_doc->setModifiedOnDiskWarning(true);
            if (QAction* a = kteAction(m_view, "view_auto_reload")) a->setChecked(false);
        });
    
    // Focus guard: install event filter on all KTE children
    setFocusPolicy(Qt::ClickFocus);
    installFocusGuard();
}

EditorWidget::~EditorWidget() {
    // Don't prompt here — DC calls this every time you move to another file.
    // The save prompt is triggered inside loadFile() before replacing the document.
}

void EditorWidget::installFocusGuard() {
    // Install ourselves as event filter on all children so we can intercept
    // FocusIn events that would steal keyboard focus from DC's file panel.
    const auto children = findChildren<QWidget*>();
    for (QWidget *child : children) {
        child->installEventFilter(this);
    }
    m_view->installEventFilter(this);
    if (m_view->focusProxy()) m_view->focusProxy()->installEventFilter(this);
}

bool EditorWidget::eventFilter(QObject *obj, QEvent *event) {
    // Prevent DC from stealing keystrokes the user intends for the editor
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (parentWidget()) parentWidget()->setFocus();
            return true;
        }
        event->accept();
        return true;
    }
    
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            if (parentWidget()) parentWidget()->setFocus();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Alt) {
            m_view->setBlockSelection(true);
        } else if (m_view->blockSelection()) {
            m_view->setBlockSelection(false);
        }
    }
    
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (!(me->modifiers() & Qt::AltModifier)) {
            m_view->setBlockSelection(false);
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

bool EditorWidget::loadFile(const QString &filePath) {
    if (!m_doc) return false;

    // Prompt to save if there are unsaved changes from a previous file
    if (m_doc->isModified()) {
        QMessageBox::StandardButton resBtn = QMessageBox::question(
            nullptr, "Unsaved Changes",
            "The file has been modified.\nDo you want to save your changes?",
            QMessageBox::Save | QMessageBox::Discard,
            QMessageBox::Save
        );
        if (resBtn == QMessageBox::Save) {
            m_doc->documentSave();
        }
    }

    // Set config BEFORE opening to pre-empt global katepartrc defaults
    m_doc->setConfigValue("remove-spaces", 0);
    m_doc->setConfigValue("auto-reload-on-external-changes", false);
    m_doc->setModifiedOnDiskWarning(true);
    
    bool success = m_doc->openUrl(QUrl::fromLocalFile(filePath));
    if (!success) {
        m_doc->setText("Error: Could not open file via KIO: " + filePath);
        return true;
    }
    
    // Re-apply AFTER openUrl — KTE resets configs from global katepartrc on load
    m_doc->setConfigValue("remove-spaces", 0);
    m_doc->setConfigValue("auto-reload-on-external-changes", false);
    m_doc->setModifiedOnDiskWarning(true);
    if (QAction* a = kteAction(m_view, "view_auto_reload")) a->setChecked(false);
    
    // Read-only by default
    m_doc->setReadWrite(false);
    m_view->setContextMenu(m_view->defaultContextMenu());
    m_view->setCursorPosition(KTextEditor::Cursor(0, 0));
    m_view->setStatusBarEnabled(false);
    
    updateStatusBar();
    return true;
}

void EditorWidget::copySelection() {
    if (QAction* a = kteAction(m_view, "edit_copy")) a->trigger();
}

void EditorWidget::selectAll() {
    if (QAction* a = kteAction(m_view, "edit_select_all")) a->trigger();
}

void EditorWidget::setupMenu() {
    // File
    QMenu *fileMenu = m_menuBar->addMenu("&File");
    QAction *saveAction = new QAction("Save", this);
    connect(saveAction, &QAction::triggered, m_doc, &KTextEditor::Document::save);
    fileMenu->addAction(saveAction);
    
    QAction *saveAsAction = new QAction("Save As...", this);
    connect(saveAsAction, &QAction::triggered, m_doc, &KTextEditor::Document::documentSaveAs);
    fileMenu->addAction(saveAsAction);
    
    QAction *reloadAction = new QAction("Reload from Disk", this);
    connect(reloadAction, &QAction::triggered, m_doc, &KTextEditor::Document::documentReload);
    fileMenu->addAction(reloadAction);
    
    // Native "Save As with Encoding" action
    if (QAction* a = kteAction(m_view, "file_save_as_with_encoding")) fileMenu->addAction(a);
    
    // Native Encoding submenu
    if (QAction* a = kteAction(m_view, "set_encoding")) fileMenu->addAction(a);
    
    // Edit
    QMenu *editMenu = m_menuBar->addMenu("&Edit");
    if (QAction* a = kteAction(m_view, "edit_undo")) editMenu->addAction(a);
    if (QAction* a = kteAction(m_view, "edit_redo")) editMenu->addAction(a);
    editMenu->addSeparator();
    if (QAction* a = kteAction(m_view, "edit_cut")) editMenu->addAction(a);
    if (QAction* a = kteAction(m_view, "edit_copy")) editMenu->addAction(a);
    if (QAction* a = kteAction(m_view, "edit_paste")) editMenu->addAction(a);
    editMenu->addSeparator();
    if (QAction* a = kteAction(m_view, "edit_select_all")) editMenu->addAction(a);
    
    // Search
    QMenu *searchMenu = m_menuBar->addMenu("&Search");
    if (QAction* a = kteAction(m_view, "edit_find")) searchMenu->addAction(a);
    if (QAction* a = kteAction(m_view, "edit_replace")) searchMenu->addAction(a);
    if (QAction* a = kteAction(m_view, "go_goto_line")) searchMenu->addAction(a);
    
    // Tools
    QMenu *toolsMenu = m_menuBar->addMenu("&Tools");
    toolsMenu->addAction("Convert to UPPERCASE", this, &EditorWidget::convertToUpper);
    toolsMenu->addAction("Convert to lowercase", this, &EditorWidget::convertToLower);
    toolsMenu->addAction("Convert to Title Case", this, &EditorWidget::convertToTitleCase);
    toolsMenu->addAction("Convert to Proper case", this, &EditorWidget::convertToProperCase);
    toolsMenu->addAction("Convert to camelCase", this, &EditorWidget::convertToCamelCase);
    toolsMenu->addAction("Convert to snail_case", this, &EditorWidget::convertToSnailCase);
    toolsMenu->addSeparator();
    
    QMenu* eolMenu = toolsMenu->addMenu("Convert EOL");
    if (QAction* a = kteAction(m_view, "set_eol")) eolMenu->addAction(a);
    
    toolsMenu->addSeparator();
    toolsMenu->addAction("Sort Lines", this, &EditorWidget::sortLines);
    // Native trim actions from KTE scripts
    if (QAction* a = kteAction(m_view, "tools_scripts_rtrim")) toolsMenu->addAction(a);
    if (QAction* a = kteAction(m_view, "tools_scripts_ltrim")) toolsMenu->addAction(a);
    
    // View
    QMenu *viewMenu = m_menuBar->addMenu("&View");
    if (QAction* a = kteAction(m_view, "view_dynamic_word_wrap")) { m_actionWordWrap = a; viewMenu->addAction(a); }
    if (QAction* a = kteAction(m_view, "view_line_numbers")) viewMenu->addAction(a);
    if (QAction* a = kteAction(m_view, "view_folding_markers")) viewMenu->addAction(a);
    
    // "Show Hidden Characters" — cycles view_show_whitespaces to "All" mode (mode 2)
    // The action cycles: None(0) -> Trailing(1) -> All(2) -> None(0) on each trigger.
    // katepartrc has Show Spaces=2, so the action starts checked in All mode.
    // We manage our own checked state and drive the action to the right mode.
    if (QAction* wsAct = kteAction(m_view, "view_show_whitespaces")) {
        wsAct->setText("Show Hidden Characters");
        // Wrap it: our custom action tracks intent; we sync the native action to All/None
        QAction* showHiddenAction = new QAction("Show Hidden Characters", this);
        showHiddenAction->setCheckable(true);
        // Sync initial state from native action
        showHiddenAction->setChecked(wsAct->isChecked());
        connect(showHiddenAction, &QAction::toggled, this, [this, wsAct](bool checked) {
            // Drive the native action to the desired mode:
            // checked=true  → we want All (mode 2): trigger until isChecked() AND mode is All
            // checked=false → we want None (mode 0): trigger until !isChecked()
            if (checked) {
                // Trigger up to 3 times to reach All mode
                // Mode transitions: None->Trailing->All
                // If currently None (unchecked), 2 triggers reach All
                // If currently Trailing (checked, mode 1), 1 trigger reaches All
                for (int i = 0; i < 3; ++i) {
                    wsAct->trigger();
                    // Check: view_show_whitespaces checked + mode is All
                    // We detect All mode by triggering once more and seeing it goes to None
                    // Simpler: just trigger twice from None to guarantee All
                    if (i == 1) break; // After 2 triggers from None we're at All
                }
            } else {
                // Drive to None: trigger until unchecked
                for (int i = 0; i < 3 && wsAct->isChecked(); ++i)
                    wsAct->trigger();
            }
        });
        viewMenu->addAction(showHiddenAction);
    }
    
    viewMenu->addSeparator();
    QMenu *zoomMenu = viewMenu->addMenu("Zoom");
    zoomMenu->addAction("Zoom In", this, &EditorWidget::zoomIn);
    zoomMenu->addAction("Zoom Out", this, &EditorWidget::zoomOut);
    zoomMenu->addAction("Reset Zoom", this, &EditorWidget::zoomReset);
    viewMenu->addSeparator();
    
    // Native read-only toggle
    if (QAction* a = kteAction(m_view, "tools_toggle_write_lock")) {
        m_actionReadOnly = a;
        viewMenu->addAction(a);
    } else {
        m_actionReadOnly = new QAction("Read-Only Mode", this);
        m_actionReadOnly->setCheckable(true);
        connect(m_actionReadOnly, &QAction::toggled, this, [this](bool checked){ if (m_doc) m_doc->setReadWrite(!checked); });
        viewMenu->addAction(m_actionReadOnly);
    }
    
    // Native block selection toggle
    if (QAction* a = kteAction(m_view, "set_verticalSelect")) viewMenu->addAction(a);
    
    // Native highlighting/mode menus
    if (QAction* a = kteAction(m_view, "tools_highlighting")) viewMenu->addAction(a);
    if (QAction* a = kteAction(m_view, "tools_mode")) viewMenu->addAction(a);
}

void EditorWidget::setupToolBar() {
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(20, 20));

    QAction *saveAction = new QAction(QIcon::fromTheme("document-save"), "Save", this);
    connect(saveAction, &QAction::triggered, m_doc, &KTextEditor::Document::save);
    m_toolbar->addAction(saveAction);
    
    m_toolbar->addSeparator();

    QAction *undoAction = new QAction(QIcon::fromTheme("edit-undo"), "Undo", this);
    connect(undoAction, &QAction::triggered, this, [this]() {
        if (QAction* a = kteAction(m_view, "edit_undo")) a->trigger();
    });
    m_toolbar->addAction(undoAction);
    
    QAction *redoAction = new QAction(QIcon::fromTheme("edit-redo"), "Redo", this);
    connect(redoAction, &QAction::triggered, this, [this]() {
        if (QAction* a = kteAction(m_view, "edit_redo")) a->trigger();
    });
    m_toolbar->addAction(redoAction);

    m_toolbar->addSeparator();

    QAction *findAction = new QAction(QIcon::fromTheme("edit-find"), "Find", this);
    connect(findAction, &QAction::triggered, this, [this]() {
        if (QAction* a = kteAction(m_view, "edit_find")) a->trigger();
    });
    m_toolbar->addAction(findAction);
    
    QAction *replaceAction = new QAction(QIcon::fromTheme("edit-find-replace"), "Replace", this);
    connect(replaceAction, &QAction::triggered, this, [this]() {
        if (QAction* a = kteAction(m_view, "edit_replace")) a->trigger();
    });
    m_toolbar->addAction(replaceAction);
    
    m_toolbar->addSeparator();
    
    // Read-only toolbar button
    if (m_actionReadOnly) {
        m_actionReadOnly->setIcon(QIcon::fromTheme("object-locked"));
        m_toolbar->addAction(m_actionReadOnly);
    }
    
    if (m_actionWordWrap) {
        m_actionWordWrap->setIcon(QIcon::fromTheme("format-text-direction-ltr"));
        m_toolbar->addAction(m_actionWordWrap);
    }
}

void EditorWidget::setupStatusBar() {
    m_statusBar = new QWidget(this);
    m_statusLayout = new QHBoxLayout(m_statusBar);
    m_statusLayout->setContentsMargins(6, 2, 6, 2);
    
    m_statusPosition = new QLabel("Line: 1  Col: 1", m_statusBar);
    m_statusEncoding = new QLabel("UTF-8", m_statusBar);
    m_statusSyntax = new QLabel("Plain Text", m_statusBar);
    m_statusIndent = new QLabel("Spaces: 4", m_statusBar);
    m_statusMode = new QLabel("INS", m_statusBar);
    m_statusZoom = new QLabel("Zoom: 10pt", m_statusBar);
    
    m_statusLayout->addWidget(m_statusPosition);
    m_statusLayout->addStretch();
    m_statusLayout->addWidget(m_statusSyntax);
    m_statusLayout->addSpacing(15);
    m_statusLayout->addWidget(m_statusIndent);
    m_statusLayout->addSpacing(15);
    m_statusLayout->addWidget(m_statusEncoding);
    m_statusLayout->addSpacing(15);
    m_statusLayout->addWidget(m_statusMode);
    m_statusLayout->addSpacing(15);
    m_statusLayout->addWidget(m_statusZoom);
    
    m_statusBar->setStyleSheet("QWidget { background: transparent; border-top: 1px solid #555; } QLabel { font-size: 11px; }");
}

void EditorWidget::updateStatusBar() {
    if (!m_doc || !m_view) return;
    
    m_statusEncoding->setText(m_doc->encoding());
    m_statusSyntax->setText(m_doc->mode());
    m_statusMode->setText(m_doc->isReadWrite() ? "INS" : "R/O");
    m_statusZoom->setText(QString("Zoom: %1pt").arg(m_view->font().pointSize()));
}

void EditorWidget::onCursorPositionChanged() {
    if (!m_view) return;
    auto pos = m_view->cursorPosition();
    m_statusPosition->setText(QString("Line: %1  Col: %2").arg(pos.line() + 1).arg(pos.column() + 1));
}

void EditorWidget::onDocumentModeChanged() {
    updateStatusBar();
}

void EditorWidget::convertToUpper() {
    if (QAction* a = kteAction(m_view, "tools_uppercase")) { a->trigger(); return; }
    if (m_view->selection()) {
        m_doc->replaceText(m_view->selectionRange(), m_view->selectionText().toUpper());
    }
}

void EditorWidget::convertToLower() {
    if (QAction* a = kteAction(m_view, "tools_lowercase")) { a->trigger(); return; }
    if (m_view->selection()) {
        m_doc->replaceText(m_view->selectionRange(), m_view->selectionText().toLower());
    }
}

void EditorWidget::sortLines() {
    if (!m_view || !m_view->selection()) return;
    auto range = m_view->selectionRange();
    int startLine = range.start().line();
    int endLine = range.end().line();
    if (range.end().column() == 0 && endLine > startLine) endLine--;
    
    QStringList lines;
    for (int i = startLine; i <= endLine; ++i) {
        lines << m_doc->line(i);
    }
    lines.sort();
    
    for (int i = startLine; i <= endLine; ++i) {
        m_doc->replaceText(KTextEditor::Range(i, 0, i, m_doc->lineLength(i)), lines[i - startLine]);
    }
}

void EditorWidget::trimTrailingSpaces() {
    if (!m_doc) return;
    for (int i = 0; i < m_doc->lines(); ++i) {
        QString line = m_doc->line(i);
        int newLen = line.length();
        while (newLen > 0 && line[newLen-1].isSpace()) newLen--;
        if (newLen < line.length()) {
            m_doc->replaceText(KTextEditor::Range(i, newLen, i, line.length()), "");
        }
    }
}

void EditorWidget::setEncoding(const QString& encoding) {
    if (m_doc) {
        m_doc->setEncoding(encoding);
        m_doc->documentReload();
        updateStatusBar();
    }
}

// Zoom: manipulate the view's font directly since KTE has no public zoom actions
void EditorWidget::zoomIn() {
    if (!m_view) return;
    QFont f = m_view->font();
    f.setPointSize(f.pointSize() + 1);
    m_view->setFont(f);
    updateStatusBar();
}

void EditorWidget::zoomOut() {
    if (!m_view) return;
    QFont f = m_view->font();
    if (f.pointSize() > 4) {
        f.setPointSize(f.pointSize() - 1);
        m_view->setFont(f);
    }
    updateStatusBar();
}

void EditorWidget::zoomReset() {
    if (!m_view) return;
    QFont f = m_view->font();
    f.setPointSize(m_zoomLevel); // Reset to the initial size
    m_view->setFont(f);
    updateStatusBar();
}

void EditorWidget::convertToTitleCase() {
    if (!m_view || !m_view->selection()) return;
    QString text = m_view->selectionText();
    QString result;
    bool newWord = true;
    for (int i = 0; i < text.length(); ++i) {
        if (!text[i].isLetter()) {
            result += text[i];
            newWord = true;
        } else {
            if (newWord) { result += text[i].toUpper(); newWord = false; }
            else { result += text[i].toLower(); }
        }
    }
    m_doc->replaceText(m_view->selectionRange(), result);
}

void EditorWidget::convertToProperCase() {
    if (!m_view || !m_view->selection()) return;
    QString text = m_view->selectionText();
    QString result;
    bool newWord = true;
    for (int i = 0; i < text.length(); ++i) {
        if (!text[i].isLetter()) {
            result += text[i];
            newWord = true;
        } else {
            if (newWord) { result += text[i].toUpper(); newWord = false; }
            else { result += text[i]; } // Preserve original
        }
    }
    m_doc->replaceText(m_view->selectionRange(), result);
}

void EditorWidget::convertToCamelCase() {
    if (!m_view || !m_view->selection()) return;
    QString text = m_view->selectionText();
    QString result;
    bool capitalizeNext = false;
    bool firstChar = true;
    for (int i = 0; i < text.length(); ++i) {
        if (!text[i].isLetterOrNumber()) {
            capitalizeNext = true;
        } else {
            if (firstChar) {
                result += text[i].toLower();
                firstChar = false;
            } else if (capitalizeNext) {
                result += text[i].toUpper();
                capitalizeNext = false;
            } else {
                result += text[i].toLower();
            }
        }
    }
    m_doc->replaceText(m_view->selectionRange(), result);
}

void EditorWidget::convertToSnailCase() {
    if (!m_view || !m_view->selection()) return;
    QString text = m_view->selectionText();
    QString result;
    for (int i = 0; i < text.length(); ++i) {
        if (text[i].isUpper() && i > 0 && text[i-1].isLower()) {
            result += '_';
            result += text[i].toLower();
        } else if (!text[i].isLetterOrNumber()) {
            result += '_';
        } else {
            result += text[i].toLower();
        }
    }
    m_doc->replaceText(m_view->selectionRange(), result);
}

void EditorWidget::convertEolToWin() {
    if (!m_doc) return;
    QString txt = m_doc->text();
    txt.replace("\r\n", "\n");
    txt.replace("\n", "\r\n");
    m_doc->setText(txt);
}

void EditorWidget::convertEolToLinux() {
    if (!m_doc) return;
    QString txt = m_doc->text();
    txt.replace("\r\n", "\n");
    m_doc->setText(txt);
}

void EditorWidget::toggleHiddenCharacters() {
    // Deprecated — now using native view_show_whitespaces action via kteAction()
}
