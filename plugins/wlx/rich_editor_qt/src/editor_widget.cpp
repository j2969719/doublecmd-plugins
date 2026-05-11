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
#include <QAbstractButton>
#include <QPushButton>
#include <QFrame>
#include <QToolButton>
#include <QTemporaryFile>
#include <QProcess>
#include <QTextEdit>
#include <QDialog>
#include <QFontDatabase>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <algorithm>

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

static void forceActionChecked(QAction *a, bool checked) {
    if (!a) return;
    if (!a->isCheckable()) {
        if (checked) a->trigger();
        return;
    }
    // Some KTE actions only update state when triggered; setChecked() is not reliable.
    for (int i = 0; i < 4 && a->isChecked() != checked; ++i) {
        a->trigger();
    }
}

EditorWidget::EditorWidget(QWidget *parent)
    : QWidget(parent),
      m_doc(nullptr),
      m_view(nullptr),
      m_actionReadOnly(nullptr),
      m_actionWordWrap(nullptr),
      m_actionShowHidden(nullptr),
      m_zoomLevel(10),
      m_diskChangeBar(nullptr),
      m_diskChangeLabel(nullptr),
      m_diskChangeEnableAutoReload(nullptr),
      m_diskChangeViewDiff(nullptr),
      m_diskChangeReload(nullptr),
      m_diskChangeIgnore(nullptr),
      m_isActive(false),
      m_isRestoringFocus(false)
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
    // Disable swap files as early as possible before the view is created.
    forceDisableAutoReload();
    m_view = m_doc->createView(this);
    m_zoomLevel = m_view->font().pointSize();
    
    // Create UI Elements
    m_menuBar = new QMenuBar(this);
    // Unmissable marker so we can confirm which binary DC is running.
    m_menuBar->setNativeMenuBar(false);
    m_toolbar = new QToolBar(this);
    setupStatusBar();

    // Inline notification bar (hidden by default)
    m_diskChangeBar = new QFrame(this);
    m_diskChangeBar->setObjectName(QStringLiteral("DiskChangeBar"));
    m_diskChangeBar->setVisible(false);
    auto *barLayout = new QHBoxLayout(m_diskChangeBar);
    barLayout->setContentsMargins(8, 4, 8, 4);
    barLayout->setSpacing(8);
    m_diskChangeLabel = new QLabel(m_diskChangeBar);
    m_diskChangeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_diskChangeEnableAutoReload = new QToolButton(m_diskChangeBar);
    m_diskChangeEnableAutoReload->setText(QStringLiteral("Enable Auto Reload"));
    m_diskChangeEnableAutoReload->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_diskChangeViewDiff = new QToolButton(m_diskChangeBar);
    m_diskChangeViewDiff->setText(QStringLiteral("View Difference"));
    m_diskChangeViewDiff->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_diskChangeReload = new QPushButton(QStringLiteral("Reload"), m_diskChangeBar);
    m_diskChangeIgnore = new QPushButton(QStringLiteral("Ignore"), m_diskChangeBar);
    barLayout->addWidget(m_diskChangeLabel, 1);
    barLayout->addWidget(m_diskChangeEnableAutoReload);
    barLayout->addWidget(m_diskChangeViewDiff);
    barLayout->addWidget(m_diskChangeReload);
    barLayout->addWidget(m_diskChangeIgnore);
    m_diskChangeBar->setStyleSheet(
        "QFrame#DiskChangeBar {"
        "  background: #2b2b2b;"
        "  border-bottom: 1px solid #444;"
        "}"
        "QLabel { color: #ddd; }"
        "QPushButton { padding: 3px 10px; }"
        "QToolButton { padding: 3px 8px; color: #ddd; border: 1px solid #555; border-radius: 3px; }"
        "QToolButton:hover { border-color: #777; }"
    );

    // Setup Actions and UI
    setupMenu();
    setupToolBar();

    // Layout Assembly
    m_layout->addWidget(m_menuBar);
    m_layout->addWidget(m_toolbar);
    m_layout->addWidget(m_diskChangeBar);
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
    
    // Make sure the editor widget itself can receive focus
    setFocusPolicy(Qt::StrongFocus);
    m_savedSelectionValid = false;
    
    // Disable auto-reload inside the document/view (do NOT write global katepartrc here).
    forceDisableAutoReload();
    
    // --- Trailing-space stripping: OFF ---
    m_doc->setConfigValue("remove-spaces", 0);
    
    connect(m_view, &KTextEditor::View::cursorPositionChanged, this, &EditorWidget::onCursorPositionChanged);
    connect(m_doc, &KTextEditor::Document::readWriteChanged, this, &EditorWidget::onDocumentModeChanged);
    connect(m_doc, &KTextEditor::Document::modeChanged, this, &EditorWidget::onDocumentModeChanged);
    connect(m_doc, &KTextEditor::Document::documentSavedOrUploaded, this, [this](KTextEditor::Document *doc, bool) {
        if (doc != m_doc || !m_savedSelectionValid || !m_view) {
            return;
        }

        m_view->setCursorPosition(m_savedCursor);
        m_view->setSelection(m_savedSelection);
        m_savedSelectionValid = false;
    });

    // Diagnostics: capture what triggers reloads / modified-on-disk state.
    connect(m_doc, &KTextEditor::Document::aboutToReload, this, [this](KTextEditor::Document *d) {
    });
    connect(m_doc, &KTextEditor::Document::reloaded, this, [this](KTextEditor::Document *d) {
    });
    connect(m_doc, &KTextEditor::Document::textChanged, this, [this](KTextEditor::Document *d) {
    });
    connect(m_doc, &KTextEditor::Document::modifiedChanged, this, [this](KTextEditor::Document *d) {
    });
    
    // External-change detection (replace silent reload with an explicit prompt).
    connect(m_doc, &KTextEditor::Document::modifiedOnDisk, this,
        [this](KTextEditor::Document* doc, bool modified, KTextEditor::Document::ModifiedOnDiskReason) {
            if (!modified || !doc) return;
            forceDisableAutoReload();

            const QString path = doc->url().isLocalFile()
                ? doc->url().toLocalFile()
                : doc->url().toDisplayString();

            m_diskChangeLabel->setText(QStringLiteral("The file \"%1\" was modified on disk.").arg(path));
            m_diskChangeBar->setVisible(true);
        });

    connect(m_diskChangeReload, &QPushButton::clicked, this, [this]() {
        if (!m_doc) return;
        hideDiskChangeBar();
        m_doc->documentReload();
    });
    connect(m_diskChangeIgnore, &QPushButton::clicked, this, [this]() {
        hideDiskChangeBar();
    });
    connect(m_diskChangeEnableAutoReload, &QToolButton::clicked, this, [this]() {
        enableAutoReload();
        hideDiskChangeBar();
    });

    // Focus guard: install event filter on all KTE children and watch for outside clicks.
    setFocusPolicy(Qt::ClickFocus);
    installFocusGuard();

    auto isWithinEditor = [this](QWidget *widget) {
        if (!widget) return false;
        if (widget == this || widget == m_view || widget == m_view->focusProxy()) return true;
        return isAncestorOf(widget);
    };

    auto isInternalKteFocusWidget = [](QWidget *widget) {
        if (!widget) return false;
        const QByteArray className = widget->metaObject()->className();
        return QString::fromLatin1(className).contains(QLatin1String("Kate")) ||
               QString::fromLatin1(className).contains(QLatin1String("QAbstractScrollArea"));
    };

    connect(qApp, &QApplication::focusChanged, this, [this, isWithinEditor, isInternalKteFocusWidget](QWidget *old, QWidget *now) {
        if (!m_isActive || m_isRestoringFocus) return;
        if (!old || !isWithinEditor(old)) return;
        if (isWithinEditor(now)) return;
        if (now == this || isInternalKteFocusWidget(now)) {
            m_isRestoringFocus = true;
            QWidget *focusTarget = m_view->focusProxy() ? m_view->focusProxy() : m_view;
            focusTarget->setFocus(Qt::OtherFocusReason);
            m_isRestoringFocus = false;
        }
    });

    qApp->installEventFilter(this);
}

EditorWidget::~EditorWidget() {
    qApp->removeEventFilter(this);
    // Don't prompt here — DC calls this every time you move to another file.
    // The save prompt is triggered inside loadFile() before replacing the document.
}

void EditorWidget::installFocusGuard() {
    // Install ourselves as event filter on the main view and focus proxy
    // to intercept FocusIn events that would steal keyboard focus from DC's file panel.
    m_view->installEventFilter(this);
    if (m_view->focusProxy()) {
        m_view->focusProxy()->installEventFilter(this);
    }
}

KTextEditor::Cursor EditorWidget::findWordStart(const KTextEditor::Cursor &cursor) const {
    if (!m_doc) return cursor;

    int line = cursor.line();
    int col = cursor.column();

    // At the start of a line: stop at end of previous line (line boundary).
    if (col == 0) {
        if (line == 0) return cursor;
        int prevLine = line - 1;
        return KTextEditor::Cursor(prevLine, m_doc->lineLength(prevLine));
    }

    // Within a line: skip backwards within this line only.
    QString text = m_doc->line(line);

    // If currently on whitespace, skip whitespace backwards (stay on this line).
    if (col > 0 && text[col - 1].isSpace()) {
        while (col > 0 && text[col - 1].isSpace()) {
            col--;
        }
        // If we consumed only whitespace and landed at col 0, stop here.
        if (col == 0) return KTextEditor::Cursor(line, 0);
    }

    // Skip word characters backwards to find word start.
    while (col > 0 && !text[col - 1].isSpace()) {
        col--;
    }

    return KTextEditor::Cursor(line, col);
}

KTextEditor::Cursor EditorWidget::findWordEnd(const KTextEditor::Cursor &cursor) const {
    if (!m_doc) return cursor;

    int line = cursor.line();
    int col = cursor.column();
    const int lastLine = m_doc->lines() - 1;
    int lineLen = m_doc->lineLength(line);

    // At the end of a line: stop at beginning of next line (line boundary).
    if (col >= lineLen) {
        if (line >= lastLine) return cursor;
        return KTextEditor::Cursor(line + 1, 0);
    }

    // Within a line: skip forwards within this line only.
    QString text = m_doc->line(line);

    // If currently on whitespace, skip whitespace forwards (stay on this line).
    if (col < lineLen && text[col].isSpace()) {
        while (col < lineLen && text[col].isSpace()) {
            col++;
        }
        // If we consumed only whitespace and landed at end-of-line, stop here.
        if (col >= lineLen) return KTextEditor::Cursor(line, lineLen);
    }

    // Skip word characters forwards to find word end.
    while (col < lineLen && !text[col].isSpace()) {
        col++;
    }

    return KTextEditor::Cursor(line, col);
}

bool EditorWidget::eventFilter(QObject *obj, QEvent *event) {
    // Detect clicks outside our editor panel so Double Commander can regain focus.
    if (event->type() == QEvent::MouseButtonPress && m_isActive) {
        auto *me = static_cast<QMouseEvent*>(event);
        const QPoint gp = me->globalPosition().toPoint();
        const QRect gr(mapToGlobal(QPoint(0, 0)), size());
        if (!gr.contains(gp)) {
            setActive(false);
        }
    }

    // Only handle key events from our main view and focus proxy.
    if (obj != m_view && obj != m_view->focusProxy()) {
        return QWidget::eventFilter(obj, event);
    }
    
    // Log all key events to diagnose delivery
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        if (event->type() == QEvent::KeyPress) {
            const int key = keyEvent->key();
            const int mods = static_cast<int>(keyEvent->modifiers());
            if (key == Qt::Key_Backspace || key == Qt::Key_Delete || key == Qt::Key_Insert ||
                (key == Qt::Key_S && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_C && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_V && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_X && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_F && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_R && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_G && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_A && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_Z && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_Y && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_W && (mods & Qt::ControlModifier)) ||
                (key == Qt::Key_B && (mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier)) ||
                key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down ||
                key == Qt::Key_Home || key == Qt::Key_End || key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
                bool triggered = false;
                
                // Try to find and trigger the corresponding action
                if (key == Qt::Key_Insert) {
                    if (QAction *a = kteAction(m_view, "set_insert")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
                    if (QAction *a = kteAction(m_view, "edit_delete")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                    if (!triggered && m_doc && m_view && m_doc->isReadWrite()) {
                        KTextEditor::Cursor cursor = m_view->cursorPosition();
                        bool hasSelection = m_view->selection();
                        if (hasSelection) {
                            const KTextEditor::Range selection = m_view->selectionRange();
                            if (m_doc->removeText(selection)) {
                                m_view->setCursorPosition(selection.start());
                                m_view->removeSelection();
                                triggered = true;
                            }
                        } else if (key == Qt::Key_Backspace) {
                            if (cursor.column() > 0) {
                                KTextEditor::Cursor start(cursor.line(), cursor.column() - 1);
                                if (m_doc->removeText(KTextEditor::Range(start, cursor))) {
                                    m_view->setCursorPosition(start);
                                    triggered = true;
                                }
                            } else if (cursor.line() > 0) {
                                int prevLine = cursor.line() - 1;
                                int prevLineLen = m_doc->lineLength(prevLine);
                                KTextEditor::Cursor start(prevLine, prevLineLen);
                                if (m_doc->removeText(KTextEditor::Range(start, cursor))) {
                                    m_view->setCursorPosition(start);
                                    triggered = true;
                                }
                            }
                        } else if (key == Qt::Key_Delete) {
                            int lineLen = m_doc->lineLength(cursor.line());
                            if (cursor.column() < lineLen) {
                                KTextEditor::Cursor end(cursor.line(), cursor.column() + 1);
                                if (m_doc->removeText(KTextEditor::Range(cursor, end))) {
                                    m_view->setCursorPosition(cursor);
                                    triggered = true;
                                }
                            } else if (cursor.line() < m_doc->lines() - 1) {
                                KTextEditor::Cursor end(cursor.line() + 1, 0);
                                if (m_doc->removeText(KTextEditor::Range(cursor, end))) {
                                    m_view->setCursorPosition(cursor);
                                    triggered = true;
                                }
                            }
                        }
                    }
                } else if (key == Qt::Key_C && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "edit_copy")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_V && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "edit_paste")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_X && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "edit_cut")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_F && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "edit_find")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_R && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "edit_replace")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_G && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "go_goto_line")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_A && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "edit_select_all")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_Z && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "edit_undo")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_Y && (mods & Qt::ControlModifier)) {
                    if (QAction *a = kteAction(m_view, "edit_redo")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_S && (mods & Qt::ControlModifier)) {
                    saveDocument();
                    triggered = true;
                } else if (key == Qt::Key_W && (mods & Qt::ControlModifier)) {
                    // Ctrl+W: toggle read-only mode
                    if (m_doc) {
                        bool nowReadWrite = !m_doc->isReadWrite();
                        m_doc->setReadWrite(nowReadWrite);
                        // Sync the toolbar/menu action if it exists
                        if (m_actionReadOnly && m_actionReadOnly->isCheckable()) {
                            m_actionReadOnly->blockSignals(true);
                            m_actionReadOnly->setChecked(!nowReadWrite);
                            m_actionReadOnly->blockSignals(false);
                        }
                        updateStatusBar();
                    }
                    triggered = true;
                } else if (key == Qt::Key_B && (mods & Qt::ControlModifier) && (mods & Qt::ShiftModifier)) {
                    if (QAction *a = kteAction(m_view, "set_verticalSelect")) {
                        if (a->isEnabled()) { a->trigger(); triggered = true; }
                    }
                } else if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down ||
                           key == Qt::Key_Home || key == Qt::Key_End || key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
                    // Handle navigation with selection (Shift), word navigation (Ctrl+Shift),
                    // and clear the selection when moving without Shift.
                    KTextEditor::Cursor cursor = m_view->cursorPosition();
                    KTextEditor::Range selection = m_view->selectionRange();
                    bool hasSelection = m_view->selection();
                    bool extendSelection = (mods & Qt::ShiftModifier);
                    if (!extendSelection && hasSelection) {
                        m_view->setSelection(KTextEditor::Range(cursor, cursor));
                        hasSelection = false;
                    }
                    
                    if (key == Qt::Key_Left) {
                        if (mods & Qt::ControlModifier) {
                            // Ctrl+Left: move to start of word or line boundary
                            KTextEditor::Cursor target = findWordStart(cursor);
                            if (extendSelection) {
                                if (hasSelection) {
                                    KTextEditor::Range newSelection = selection;
                                    if (cursor == selection.start()) {
                                        newSelection.setStart(target);
                                    } else {
                                        newSelection.setEnd(target);
                                    }
                                    m_view->setSelection(newSelection);
                                    m_view->setCursorPosition(target);
                                } else {
                                    m_view->setSelection(KTextEditor::Range(target, cursor));
                                    m_view->setCursorPosition(target);
                                }
                            } else {
                                m_view->setCursorPosition(target);
                            }
                            triggered = true;
                        } else if (extendSelection) {
                            // Shift+Left: extend selection left
                            if (cursor.column() == 0 && cursor.line() > 0) {
                                // Move to end of previous line
                                int prevLine = cursor.line() - 1;
                                int prevLineLen = m_view->document()->lineLength(prevLine);
                                if (hasSelection) {
                                    KTextEditor::Range newSelection = selection;
                                    if (cursor == selection.start()) {
                                        newSelection.setStart(KTextEditor::Cursor(prevLine, prevLineLen));
                                    } else {
                                        newSelection.setEnd(KTextEditor::Cursor(prevLine, prevLineLen));
                                    }
                                    m_view->setSelection(newSelection);
                                    cursor.setLine(prevLine);
                                    cursor.setColumn(prevLineLen);
                                    m_view->setCursorPosition(cursor);
                                } else {
                                    // Start selection
                                    KTextEditor::Cursor newPos(prevLine, prevLineLen);
                                    m_view->setSelection(KTextEditor::Range(newPos, cursor));
                                    m_view->setCursorPosition(newPos);
                                }
                            } else {
                                if (hasSelection) {
                                    KTextEditor::Range newSelection = selection;
                                    if (cursor == selection.start()) {
                                        newSelection.setStart(KTextEditor::Cursor(cursor.line(), std::max(0, cursor.column() - 1)));
                                    } else {
                                        newSelection.setEnd(KTextEditor::Cursor(cursor.line(), std::max(0, cursor.column() - 1)));
                                    }
                                    m_view->setSelection(newSelection);
                                    cursor.setColumn(std::max(0, cursor.column() - 1));
                                    m_view->setCursorPosition(cursor);
                                } else {
                                    // Start selection
                                    KTextEditor::Cursor newPos(cursor.line(), std::max(0, cursor.column() - 1));
                                    m_view->setSelection(KTextEditor::Range(newPos, cursor));
                                    m_view->setCursorPosition(newPos);
                                }
                            }
                            triggered = true;
                        } else {
                            // Plain Left
                            if (cursor.column() == 0 && cursor.line() > 0) {
                                // Move to end of previous line
                                int prevLine = cursor.line() - 1;
                                cursor.setLine(prevLine);
                                cursor.setColumn(m_view->document()->lineLength(prevLine));
                            } else {
                                cursor.setColumn(std::max(0, cursor.column() - 1));
                            }
                            m_view->setCursorPosition(cursor);
                            triggered = true;
                        }
                    } else if (key == Qt::Key_Right) {
                        if (mods & Qt::ControlModifier) {
                            // Ctrl+Right: move to end of word or line boundary
                            KTextEditor::Cursor target = findWordEnd(cursor);
                            if (extendSelection) {
                                if (hasSelection) {
                                    KTextEditor::Range newSelection = selection;
                                    if (cursor == selection.start()) {
                                        newSelection.setStart(target);
                                    } else {
                                        newSelection.setEnd(target);
                                    }
                                    m_view->setSelection(newSelection);
                                    m_view->setCursorPosition(target);
                                } else {
                                    m_view->setSelection(KTextEditor::Range(cursor, target));
                                    m_view->setCursorPosition(target);
                                }
                            } else {
                                m_view->setCursorPosition(target);
                            }
                            triggered = true;
                        } else if (extendSelection) {
                            // Shift+Right: extend selection right
                            int lineLength = m_view->document()->lineLength(cursor.line());
                            if (cursor.column() >= lineLength && cursor.line() < m_view->document()->lines() - 1) {
                                // Move to beginning of next line
                                int nextLine = cursor.line() + 1;
                                if (hasSelection) {
                                    KTextEditor::Range newSelection = selection;
                                    if (cursor == selection.start()) {
                                        newSelection.setStart(KTextEditor::Cursor(nextLine, 0));
                                    } else {
                                        newSelection.setEnd(KTextEditor::Cursor(nextLine, 0));
                                    }
                                    m_view->setSelection(newSelection);
                                    cursor.setLine(nextLine);
                                    cursor.setColumn(0);
                                    m_view->setCursorPosition(cursor);
                                } else {
                                    // Start selection
                                    KTextEditor::Cursor newPos(nextLine, 0);
                                    m_view->setSelection(KTextEditor::Range(cursor, newPos));
                                    m_view->setCursorPosition(newPos);
                                }
                            } else {
                                if (hasSelection) {
                                    KTextEditor::Range newSelection = selection;
                                    if (cursor == selection.start()) {
                                        newSelection.setStart(KTextEditor::Cursor(cursor.line(), std::min(lineLength, cursor.column() + 1)));
                                    } else {
                                        newSelection.setEnd(KTextEditor::Cursor(cursor.line(), std::min(lineLength, cursor.column() + 1)));
                                    }
                                    m_view->setSelection(newSelection);
                                    cursor.setColumn(std::min(lineLength, cursor.column() + 1));
                                    m_view->setCursorPosition(cursor);
                                } else {
                                    // Start selection
                                    KTextEditor::Cursor newPos(cursor.line(), std::min(lineLength, cursor.column() + 1));
                                    m_view->setSelection(KTextEditor::Range(cursor, newPos));
                                    m_view->setCursorPosition(newPos);
                                }
                            }
                            triggered = true;
                        } else {
                            // Plain Right
                            int lineLength = m_view->document()->lineLength(cursor.line());
                            if (cursor.column() >= lineLength && cursor.line() < m_view->document()->lines() - 1) {
                                // Move to beginning of next line
                                cursor.setLine(cursor.line() + 1);
                                cursor.setColumn(0);
                            } else {
                                cursor.setColumn(std::min(lineLength, cursor.column() + 1));
                            }
                            m_view->setCursorPosition(cursor);
                            triggered = true;
                        }
                    } else if (key == Qt::Key_Up) {
                        if (extendSelection) {
                            // Shift+Up: extend selection up
                            int newLine = std::max(0, cursor.line() - 1);
                            if (hasSelection) {
                                KTextEditor::Range newSelection = selection;
                                if (cursor == selection.start()) {
                                    newSelection.setStart(KTextEditor::Cursor(newLine, cursor.column()));
                                } else {
                                    newSelection.setEnd(KTextEditor::Cursor(newLine, cursor.column()));
                                }
                                m_view->setSelection(newSelection);
                                cursor.setLine(newLine);
                                m_view->setCursorPosition(cursor);
                            } else {
                                // Start selection
                                KTextEditor::Cursor newPos(newLine, cursor.column());
                                m_view->setSelection(KTextEditor::Range(newPos, cursor));
                                m_view->setCursorPosition(newPos);
                            }
                            triggered = true;
                        } else {
                            // Plain Up
                            cursor.setLine(std::max(0, cursor.line() - 1));
                            m_view->setCursorPosition(cursor);
                            triggered = true;
                        }
                    } else if (key == Qt::Key_Down) {
                        if (extendSelection) {
                            // Shift+Down: extend selection down
                            int maxLine = m_view->document()->lines() - 1;
                            int newLine = std::min(maxLine, cursor.line() + 1);
                            if (hasSelection) {
                                KTextEditor::Range newSelection = selection;
                                if (cursor == selection.start()) {
                                    newSelection.setStart(KTextEditor::Cursor(newLine, cursor.column()));
                                } else {
                                    newSelection.setEnd(KTextEditor::Cursor(newLine, cursor.column()));
                                }
                                m_view->setSelection(newSelection);
                                cursor.setLine(newLine);
                                m_view->setCursorPosition(cursor);
                            } else {
                                // Start selection
                                KTextEditor::Cursor newPos(newLine, cursor.column());
                                m_view->setSelection(KTextEditor::Range(cursor, newPos));
                                m_view->setCursorPosition(newPos);
                            }
                            triggered = true;
                        } else {
                            // Plain Down
                            int maxLine = m_view->document()->lines() - 1;
                            cursor.setLine(std::min(maxLine, cursor.line() + 1));
                            m_view->setCursorPosition(cursor);
                            triggered = true;
                        }
                    } else if (key == Qt::Key_Home) {
                        if (mods & Qt::ControlModifier) {
                            // Ctrl+Shift+Home: select to document start
                            if (extendSelection) {
                                if (hasSelection) {
                                    KTextEditor::Range newSelection = selection;
                                    if (cursor == selection.start()) {
                                        newSelection.setStart(KTextEditor::Cursor(0, 0));
                                    } else {
                                        newSelection.setEnd(KTextEditor::Cursor(0, 0));
                                    }
                                    m_view->setSelection(newSelection);
                                    m_view->setCursorPosition(KTextEditor::Cursor(0, 0));
                                } else {
                                    m_view->setSelection(KTextEditor::Range(KTextEditor::Cursor(0, 0), cursor));
                                    m_view->setCursorPosition(KTextEditor::Cursor(0, 0));
                                }
                                triggered = true;
                            } else {
                                // Ctrl+Home: move to document start
                                m_view->setCursorPosition(KTextEditor::Cursor(0, 0));
                                triggered = true;
                            }
                        } else if (extendSelection) {
                            // Shift+Home: select to line start
                            if (hasSelection) {
                                KTextEditor::Range newSelection = selection;
                                if (cursor == selection.start()) {
                                    newSelection.setStart(KTextEditor::Cursor(cursor.line(), 0));
                                } else {
                                    newSelection.setEnd(KTextEditor::Cursor(cursor.line(), 0));
                                }
                                m_view->setSelection(newSelection);
                                cursor.setColumn(0);
                                m_view->setCursorPosition(cursor);
                            } else {
                                m_view->setSelection(KTextEditor::Range(KTextEditor::Cursor(cursor.line(), 0), cursor));
                                cursor.setColumn(0);
                                m_view->setCursorPosition(cursor);
                            }
                            triggered = true;
                        } else {
                            // Plain Home
                            cursor.setColumn(0);
                            m_view->setCursorPosition(cursor);
                            triggered = true;
                        }
                    } else if (key == Qt::Key_End) {
                        if (mods & Qt::ControlModifier) {
                            // Ctrl+Shift+End: select to document end
                            if (extendSelection) {
                                int lastLine = m_view->document()->lines() - 1;
                                int lastCol = m_view->document()->lineLength(lastLine);
                                KTextEditor::Cursor docEnd(lastLine, lastCol);
                                if (hasSelection) {
                                    KTextEditor::Range newSelection = selection;
                                    if (cursor == selection.start()) {
                                        newSelection.setStart(docEnd);
                                    } else {
                                        newSelection.setEnd(docEnd);
                                    }
                                    m_view->setSelection(newSelection);
                                    m_view->setCursorPosition(docEnd);
                                } else {
                                    m_view->setSelection(KTextEditor::Range(cursor, docEnd));
                                    m_view->setCursorPosition(docEnd);
                                }
                                triggered = true;
                            } else {
                                // Ctrl+End: move to document end
                                int lastLine = m_view->document()->lines() - 1;
                                m_view->setCursorPosition(KTextEditor::Cursor(lastLine, m_view->document()->lineLength(lastLine)));
                                triggered = true;
                            }
                        } else if (extendSelection) {
                            // Shift+End: select to line end
                            int lineLength = m_view->document()->lineLength(cursor.line());
                            if (hasSelection) {
                                KTextEditor::Range newSelection = selection;
                                if (cursor == selection.start()) {
                                    newSelection.setStart(KTextEditor::Cursor(cursor.line(), lineLength));
                                } else {
                                    newSelection.setEnd(KTextEditor::Cursor(cursor.line(), lineLength));
                                }
                                m_view->setSelection(newSelection);
                                cursor.setColumn(lineLength);
                                m_view->setCursorPosition(cursor);
                            } else {
                                m_view->setSelection(KTextEditor::Range(cursor, KTextEditor::Cursor(cursor.line(), lineLength)));
                                cursor.setColumn(lineLength);
                                m_view->setCursorPosition(cursor);
                            }
                            triggered = true;
                        } else {
                            // Plain End
                            cursor.setColumn(m_view->document()->lineLength(cursor.line()));
                            m_view->setCursorPosition(cursor);
                            triggered = true;
                        }
                    } else if (key == Qt::Key_PageUp) {
                        if (extendSelection) {
                            // Shift+PageUp: extend selection up by page
                            int lines = 10; // Approximate page
                            int newLine = std::max(0, cursor.line() - lines);
                            if (hasSelection) {
                                KTextEditor::Range newSelection = selection;
                                if (cursor == selection.start()) {
                                    newSelection.setStart(KTextEditor::Cursor(newLine, cursor.column()));
                                } else {
                                    newSelection.setEnd(KTextEditor::Cursor(newLine, cursor.column()));
                                }
                                m_view->setSelection(newSelection);
                                cursor.setLine(newLine);
                                m_view->setCursorPosition(cursor);
                            } else {
                                KTextEditor::Cursor newPos(newLine, cursor.column());
                                m_view->setSelection(KTextEditor::Range(newPos, cursor));
                                m_view->setCursorPosition(newPos);
                            }
                            triggered = true;
                        } else {
                            // Plain PageUp
                            cursor.setLine(std::max(0, cursor.line() - 10)); // Approximate page
                            m_view->setCursorPosition(cursor);
                            triggered = true;
                        }
                    } else if (key == Qt::Key_PageDown) {
                        if (extendSelection) {
                            // Shift+PageDown: extend selection down by page
                            int lines = 10; // Approximate page
                            int maxLine = m_view->document()->lines() - 1;
                            int newLine = std::min(maxLine, cursor.line() + lines);
                            if (hasSelection) {
                                KTextEditor::Range newSelection = selection;
                                if (cursor == selection.start()) {
                                    newSelection.setStart(KTextEditor::Cursor(newLine, cursor.column()));
                                } else {
                                    newSelection.setEnd(KTextEditor::Cursor(newLine, cursor.column()));
                                }
                                m_view->setSelection(newSelection);
                                cursor.setLine(newLine);
                                m_view->setCursorPosition(cursor);
                            } else {
                                KTextEditor::Cursor newPos(newLine, cursor.column());
                                m_view->setSelection(KTextEditor::Range(cursor, newPos));
                                m_view->setCursorPosition(newPos);
                            }
                            triggered = true;
                        } else {
                            // Plain PageDown
                            int maxLine = m_view->document()->lines() - 1;
                            cursor.setLine(std::min(maxLine, cursor.line() + 10)); // Approximate page
                            m_view->setCursorPosition(cursor);
                            triggered = true;
                        }
                    }
                }
                
                if (triggered) {
                    return true;
                }
            }
        }
    }
    
    // Deactivate on outside click so DC can regain focus.
    if (event->type() == QEvent::MouseButtonPress && m_isActive) {
        auto *me = static_cast<QMouseEvent*>(event);
        const QPoint gp = me->globalPosition().toPoint();
        const QRect gr(mapToGlobal(QPoint(0, 0)), size());
        if (!gr.contains(gp)) {
            setActive(false);
            return false;
        }
    }
    
    // Activate on any click inside our panel.
    if (event->type() == QEvent::MouseButtonPress && !m_isActive) {
        auto *me = static_cast<QMouseEvent*>(event);
        const QPoint gp = me->globalPosition().toPoint();
        const QRect gr(mapToGlobal(QPoint(0, 0)), size());
        if (gr.contains(gp)) {
            setActive(true);
            QWidget *focusTarget = m_view->focusProxy() ? m_view->focusProxy() : m_view;
            focusTarget->setFocus(Qt::MouseFocusReason);
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

void EditorWidget::saveDocument() {
    if (!m_doc || !m_view) {
        return;
    }

    // Capture cursor/selection state before save
    KTextEditor::Cursor cursorBefore = m_view->cursorPosition();
    bool hadSelection = m_view->selection();
    KTextEditor::Range selBefore = hadSelection
        ? m_view->selectionRange()
        : KTextEditor::Range(cursorBefore, cursorBefore);
    bool wasReadWrite = m_doc->isReadWrite();

    // Also set the async restore in case KTE triggers an internal reload
    m_savedCursor = cursorBefore;
    m_savedSelection = selBefore;
    m_savedSelectionValid = true;

    m_doc->save();

    // Restore cursor/selection synchronously
    m_view->setCursorPosition(cursorBefore);
    if (hadSelection) {
        m_view->setSelection(selBefore);
    }
    // Preserve the read-write state that was active before saving
    m_doc->setReadWrite(wasReadWrite);
}

void EditorWidget::focusOutEvent(QFocusEvent *event) {
    if (m_isActive) {
        const auto reason = event->reason();
        if (reason == Qt::MouseFocusReason || reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason) {
            QWidget *now = qApp->focusWidget();
            if (!now || (now != m_view && now != m_view->focusProxy() && !isAncestorOf(now))) {
                setActive(false);
            }
        }
    }
    QWidget::focusOutEvent(event);
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
    forceDisableAutoReload();
    
    bool success = m_doc->openUrl(QUrl::fromLocalFile(filePath));
    if (!success) {
        m_doc->setText("Error: Could not open file via KIO: " + filePath);
        return true;
    }
    
    // Re-apply AFTER openUrl — KTE resets configs from global katepartrc on load
    m_doc->setConfigValue("remove-spaces", 0);
    forceDisableAutoReload();
    
    // Read-only by default — user can toggle via toolbar/menu/Ctrl+Shift+B lock
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

QString EditorWidget::currentFilePath() const {
    if (!m_doc) return {};
    if (!m_doc->url().isLocalFile()) return {};
    return m_doc->url().toLocalFile();
}

bool EditorWidget::isModified() const {
    return m_doc ? m_doc->isModified() : false;
}

void EditorWidget::hostSetFocus(bool focus) {
    if (focus) {
        setActive(true);
        setFocus(Qt::OtherFocusReason);
        if (m_view) m_view->setFocus(Qt::OtherFocusReason);
    } else {
        setActive(false);
    }
}

void EditorWidget::setupMenu() {
    // File
    QMenu *fileMenu = m_menuBar->addMenu("&File");
    QAction *saveAction = new QAction("Save", this);
    connect(saveAction, &QAction::triggered, this, &EditorWidget::saveDocument);
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
    eolMenu->addAction("Windows (CRLF)", this, &EditorWidget::convertEolToWin);
    eolMenu->addAction("Linux (LF)", this, &EditorWidget::convertEolToLinux);
    eolMenu->addAction("MacOS (CR)", this, &EditorWidget::convertEolToMac);
    
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
    
    // "Show Hidden Characters" (tabs/spaces/EOL markers).
    // KTextEditor/Kate can expose this as one cycling action (whitespaces) plus
    // optional separate toggles for tabs/EOL depending on framework version.
    m_actionShowHidden = new QAction(QStringLiteral("Show Hidden Characters"), this);
    m_actionShowHidden->setCheckable(true);
    connect(m_actionShowHidden, &QAction::toggled, this, [this](bool checked) {
        setHiddenCharactersVisible(checked);
    });
    viewMenu->addAction(m_actionShowHidden);
    
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
    connect(saveAction, &QAction::triggered, this, &EditorWidget::saveDocument);
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

void EditorWidget::convertEolToMac() {
    if (!m_doc) return;
    QString txt = m_doc->text();
    txt.replace("\r\n", "\r");
    txt.replace("\n", "\r");
    m_doc->setText(txt);
}

void EditorWidget::toggleHiddenCharacters() {
    // Deprecated — now using native view_show_whitespaces action via kteAction()
}

void EditorWidget::forceDisableAutoReload() {
    if (!m_doc) return;
    m_doc->setConfigValue("auto-reload-on-external-changes", false);
    // Prefer prompt-on-change behavior when available.
    m_doc->setModifiedOnDiskWarning(true);

    // Disable swap files to prevent focus loss during periodic writes
    m_doc->setConfigValue("swap-file-mode", 0); // 0 = No swap file
    m_doc->setConfigValue("swap-file-sync", 0); // Disable swap file syncing

    // Some builds only honor this through the view action; force it OFF if present.
    if (QAction *a = kteAction(m_view, "view_auto_reload")) {
        forceActionChecked(a, false);
    }
}

void EditorWidget::enableAutoReload() {
    if (!m_doc) return;
    m_doc->setConfigValue("auto-reload-on-external-changes", true);
    // If KTE honors this via an action, flip it on.
    if (QAction *a = kteAction(m_view, "view_auto_reload")) {
        forceActionChecked(a, true);
    }
}

void EditorWidget::showDiffAgainstDisk() {
    if (!m_doc) return;
    if (!m_doc->url().isLocalFile()) return;

    const QString path = m_doc->url().toLocalFile();
    QFile diskFile(path);
    if (!diskFile.open(QIODevice::ReadOnly)) return;
    const QByteArray diskBytes = diskFile.readAll();

    // Write current buffer to a temp file and diff against disk.
    QTemporaryFile tmp;
    tmp.setAutoRemove(true);
    if (!tmp.open()) return;
    const QByteArray bufBytes = m_doc->text().toUtf8();
    tmp.write(bufBytes);
    tmp.flush();

    QProcess proc;
    QStringList args;
    args << "-u"
         << "--label" << QStringLiteral("on-disk")
         << "--label" << QStringLiteral("buffer")
         << path
         << tmp.fileName();
    proc.start(QStringLiteral("diff"), args);
    proc.waitForFinished(2000);
    const QByteArray out = proc.readAllStandardOutput();
    const QByteArray err = proc.readAllStandardError();

    const QString diffText = !out.isEmpty()
        ? QString::fromLocal8Bit(out)
        : (!err.isEmpty() ? QString::fromLocal8Bit(err) : QStringLiteral("(no differences)"));

    auto *dlg = new QDialog(this);
    dlg->setWindowTitle(QStringLiteral("View Difference"));
    dlg->resize(900, 600);
    auto *layout = new QVBoxLayout(dlg);
    auto *edit = new QTextEdit(dlg);
    edit->setReadOnly(true);
    edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    edit->setPlainText(diffText);
    layout->addWidget(edit);
    dlg->setLayout(layout);
    dlg->setAttribute(Qt::WA_DeleteOnClose, true);
    dlg->show();
}

void EditorWidget::setHiddenCharactersVisible(bool visible) {
    if (!m_view) return;

    // Best-effort list of known Kate/KTextEditor action names across versions.
    // We toggle anything we find rather than relying on a single cycling action.
    struct Toggle {
        const char *name;
        bool desired;
    };

    const Toggle toggles[] = {
        // Main whitespace marker action (often cycles through modes)
        {"view_show_whitespaces", visible},
        // Tabs and EOL markers (when available as separate toggles)
        {"view_show_tabs", visible},
        {"view_show_tabulators", visible},
        {"view_show_line_breaks", visible},
        {"view_show_eol", visible},
    };

    for (const auto &t : toggles) {
        if (QAction *a = kteAction(m_view, t.name)) {
            if (a->isCheckable()) {
                forceActionChecked(a, t.desired);
            } else if (visible) {
                a->trigger();
            }
        }
    }

    // Sync the visible state from whatever primary action exists.
    if (m_actionShowHidden) {
        if (QAction *ws = kteAction(m_view, "view_show_whitespaces")) {
            m_actionShowHidden->blockSignals(true);
            m_actionShowHidden->setChecked(ws->isChecked() || visible);
            m_actionShowHidden->blockSignals(false);
        }
    }
}

void EditorWidget::hideDiskChangeBar() {
    if (m_diskChangeBar) m_diskChangeBar->setVisible(false);
}

void EditorWidget::setActive(bool active) {
    m_isActive = active;
    if (!active) {
        clearFocus();
        if (parentWidget()) parentWidget()->setFocus(Qt::OtherFocusReason);
    } else {
        setFocus(Qt::OtherFocusReason);
        QWidget *focusTarget = m_view->focusProxy() ? m_view->focusProxy() : m_view;
        focusTarget->setFocus(Qt::OtherFocusReason);
    }
}
