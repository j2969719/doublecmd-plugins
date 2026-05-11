#include "LogViewerWidget.h"
#include "LogModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QScrollBar>
#include <QTimer>
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>

// ─── LogFilterProxy ────────────────────────────────────────────────────

LogFilterProxy::LogFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent) {}

void LogFilterProxy::setRegexFilterActive(bool active) {
    beginFilterChange();
    m_regexActive = active;
    endFilterChange();
}

void LogFilterProxy::setTimeFilterActive(bool active) {
    beginFilterChange();
    m_timeActive = active;
    endFilterChange();
}

void LogFilterProxy::setTimeRange(const QDateTime &start, const QDateTime &end) {
    if (!m_timeActive) {
        m_timeStart = start;
        m_timeEnd = end;
        return;
    }
    beginFilterChange();
    m_timeStart = start;
    m_timeEnd = end;
    endFilterChange();
}

void LogFilterProxy::refreshFilter() {
    beginFilterChange();
    endFilterChange();
}

bool LogFilterProxy::filterAcceptsRow(int sourceRow,
                                      const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);
    auto *src = qobject_cast<LogModel*>(sourceModel());
    if (!src) return true;

    if (m_regexActive && !src->isMatch(sourceRow))
        return false;

    if (m_timeActive && m_timeStart.isValid() && m_timeEnd.isValid()) {
        QString line = src->lineText(sourceRow);
        QDateTime ts = LogModel::parseTimestampFromLine(line);
        if (ts.isValid()) {
            if (ts < m_timeStart || ts > m_timeEnd)
                return false;
        }
    }

    return true;
}

// ─── LogViewerWidget ───────────────────────────────────────────────────

LogViewerWidget::LogViewerWidget(QWidget *parent)
    : QWidget(parent), model(new LogModel(this))
{
    // ────────────────────────────────────────────────────────────────────
    // FOCUS LAYER 1: Preventive – NoFocus on the container itself.
    // No WA_NativeWindow, no WA_ShowWithoutActivating – those create
    // Wayland subsurface issues that are worse than the problem they solve.
    // ────────────────────────────────────────────────────────────────────
    setFocusPolicy(Qt::NoFocus);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ── Top Header ─────────────────────────────────────────────────────
    QHBoxLayout *headerLayout = new QHBoxLayout();

    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Regex search...");
    headerLayout->addWidget(searchEdit);

    btnSearchStart = new QPushButton("Search / Next", this);
    btnSearchStop  = new QPushButton("Stop", this);
    btnSearchStop->setEnabled(false);
    headerLayout->addWidget(btnSearchStart);
    headerLayout->addWidget(btnSearchStop);

    timeStart = new QDateTimeEdit(this);
    timeEnd   = new QDateTimeEdit(this);
    headerLayout->addWidget(new QLabel("From:"));
    headerLayout->addWidget(timeStart);
    headerLayout->addWidget(new QLabel("To:"));
    headerLayout->addWidget(timeEnd);

    chkFollow     = new QCheckBox("Follow", this);
    chkFilterMode = new QCheckBox("Filter", this);
    headerLayout->addWidget(chkFollow);
    headerLayout->addWidget(chkFilterMode);

    mainLayout->addLayout(headerLayout);

    // ── Filter proxy ───────────────────────────────────────────────────
    filterProxy = new LogFilterProxy(this);
    filterProxy->setSourceModel(model);

    // ── Log viewport ───────────────────────────────────────────────────
    listView = new QListView(this);
    listView->setModel(filterProxy);
    listView->setUniformItemSizes(true);
    listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    listView->setWordWrap(false);
    listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mainLayout->addWidget(listView);

    // Context menu (Copy)
    listView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu menu(this);
        QAction *copyAct = menu.addAction("Copy");
        connect(copyAct, &QAction::triggered, this, &LogViewerWidget::copySelectedLines);
        menu.exec(listView->viewport()->mapToGlobal(pos));
    });

    // ── Status bar ─────────────────────────────────────────────────────
    QHBoxLayout *statusLayout = new QHBoxLayout();
    progressBar = new QProgressBar(this);
    progressBar->hide();
    statusLabel = new QLabel("Ready.", this);
    statusLayout->addWidget(progressBar);
    statusLayout->addWidget(statusLabel);
    mainLayout->addLayout(statusLayout);

    // ────────────────────────────────────────────────────────────────────
    // FOCUS LAYER 2: Set Qt::NoFocus on ALL child widgets.
    // This prevents Tab traversal and click-to-focus from pulling keyboard
    // focus away from Double Commander's file panel.
    // We install ourselves as an event filter on every child to catch
    // any FocusIn events that bypass the policy (programmatic setFocus).
    // ────────────────────────────────────────────────────────────────────
    installFocusGuard();

    // ── Connections ────────────────────────────────────────────────────
    connect(btnSearchStart, &QPushButton::clicked,
            this, &LogViewerWidget::onSearchStartClicked);
    connect(btnSearchStop,  &QPushButton::clicked,
            this, &LogViewerWidget::onSearchStopClicked);
    connect(chkFollow, &QCheckBox::toggled,
            this, &LogViewerWidget::onFollowToggled);
    connect(chkFilterMode, &QCheckBox::toggled,
            this, &LogViewerWidget::onFilterModeToggled);
    connect(timeStart, &QDateTimeEdit::dateTimeChanged,
            this, &LogViewerWidget::onTimeRangeChanged);
    connect(timeEnd, &QDateTimeEdit::dateTimeChanged,
            this, &LogViewerWidget::onTimeRangeChanged);

    connect(model, &LogModel::searchFinished,
            this, &LogViewerWidget::onSearchFinished);
    connect(model, &LogModel::timestampsDetected,
            this, &LogViewerWidget::onTimestampsDetected);
    connect(model, &LogModel::tailUpdated,
            this, &LogViewerWidget::onTailUpdated);
}

LogViewerWidget::~LogViewerWidget() {
    qDebug() << "LogViewerWidget destroyed";
}

// ─── Focus Layer 2: installFocusGuard ──────────────────────────────────
//
// Walk the entire child tree: set NoFocus on non-input widgets.
// Input widgets (searchEdit, timeStart, timeEnd) and their internal children
// are left alone so they remain usable, but we still install our event
// filter on them for Escape/Enter handling and FocusIn interception.
//
bool LogViewerWidget::isInputWidget(QWidget *w) const {
    if (!w) return false;
    if (w == searchEdit || w == timeStart || w == timeEnd) return true;
    // Check if w is an internal child of an input widget
    if (searchEdit->isAncestorOf(w)) return true;
    if (timeStart->isAncestorOf(w)) return true;
    if (timeEnd->isAncestorOf(w)) return true;
    return false;
}

void LogViewerWidget::installFocusGuard() {
    const auto children = findChildren<QWidget*>();
    for (QWidget *child : children) {
        child->installEventFilter(this);
        // Input widgets keep their default focus policy so they remain usable
        if (!isInputWidget(child))
            child->setFocusPolicy(Qt::NoFocus);
    }
}

// ─── Focus Layer 3: save / restore ─────────────────────────────────────
//
// Call restoreFocusToDC() after any operation that may have stolen focus.
//
void LogViewerWidget::restoreFocusToDC() {
    if (m_savedFocusWidget) {
        m_savedFocusWidget->setFocus(Qt::OtherFocusReason);
    } else {
        // Last resort: clear focus from anything inside our subtree
        if (QWidget *fw = QApplication::focusWidget()) {
            if (fw == this || fw->isAncestorOf(this) || this->isAncestorOf(fw))
                fw->clearFocus();
        }
    }
}

// ─── Focus Layer 4: Global FocusIn interceptor ─────────────────────────
//
// If ANY child widget inside our subtree receives FocusIn, we immediately
// clear it — UNLESS the user has explicitly activated an input widget
// (searchEdit, timeStart, timeEnd) via mouse click.
//
bool LogViewerWidget::eventFilter(QObject *obj, QEvent *event) {
    auto *w = qobject_cast<QWidget*>(obj);

    // ── Layer 4: Intercept FocusIn on our children ─────────────────────
    if (event->type() == QEvent::FocusIn && w && this->isAncestorOf(w)) {
        // Allow focus on the active input widget and its internal children
        if (m_activeInput && (w == m_activeInput || m_activeInput->isAncestorOf(w)))
            return false;
        // Reject all other focus — restore to DC
        QTimer::singleShot(0, this, [this]() { restoreFocusToDC(); });
        return false;
    }

    // ── Handle ChildAdded: guard dynamically-created children ──────────
    if (event->type() == QEvent::ChildAdded) {
        auto *ce = static_cast<QChildEvent*>(event);
        if (auto *childWidget = qobject_cast<QWidget*>(ce->child())) {
            if (!isInputWidget(childWidget))
                childWidget->setFocusPolicy(Qt::NoFocus);
            childWidget->installEventFilter(this);
        }
    }

    // ── KeyPress handling ──────────────────────────────────────────────
    if (event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent*>(event);

        // Escape from any input widget: deactivate and restore focus to DC
        if (ke->key() == Qt::Key_Escape && m_activeInput) {
            m_activeInput = nullptr;
            restoreFocusToDC();
            return true;
        }

        // Enter in search edit: trigger search, deactivate, restore focus
        if (obj == searchEdit && (ke->key() == Qt::Key_Return ||
                                  ke->key() == Qt::Key_Enter)) {
            onSearchStartClicked();
            m_activeInput = nullptr;
            restoreFocusToDC();
            return true;
        }

        // Ctrl+C in list view: copy
        if (obj == listView && ke->matches(QKeySequence::Copy)) {
            copySelectedLines();
            return true;
        }
    }

    // ── MousePress on input widgets: activate them temporarily ─────────
    if (event->type() == QEvent::MouseButtonPress && w) {
        // Determine if click is on one of our input widgets
        QWidget *inputTarget = nullptr;
        if (w == searchEdit || searchEdit->isAncestorOf(w))
            inputTarget = searchEdit;
        else if (w == timeStart || timeStart->isAncestorOf(w))
            inputTarget = timeStart;
        else if (w == timeEnd || timeEnd->isAncestorOf(w))
            inputTarget = timeEnd;

        if (inputTarget) {
            m_activeInput = inputTarget;
            return false; // let the click through normally
        }
    }

    return QWidget::eventFilter(obj, event);
}

// ─── File loading ──────────────────────────────────────────────────────

void LogViewerWidget::loadFile(const QString& filePath) {
    qDebug() << "LogViewerWidget loading file:" << filePath;

    // FOCUS LAYER 3: Save whichever DC widget currently has focus
    m_savedFocusWidget = QApplication::focusWidget();

    currentFile = filePath;
    m_lastMatchRow = -1;
    m_lastSearchQuery.clear();
    m_activeInput = nullptr;
    statusLabel->setText(QString("Loading %1...").arg(filePath));
    model->loadFile(filePath);
    statusLabel->setText(QString("Lines: %1 | %2")
        .arg(model->lineCount()).arg(filePath));

    // FOCUS LAYER 3: Restore focus to DC after loading completes
    QTimer::singleShot(0, this, [this]() { restoreFocusToDC(); });
}

// ─── External search trigger (from ListSearchText) ─────────────────────

void LogViewerWidget::triggerSearch(const QString& searchString, int) {
    searchEdit->setText(searchString);
    onSearchStartClicked();
}

// ─── Search ────────────────────────────────────────────────────────────

void LogViewerWidget::onSearchStartClicked() {
    const QString query = searchEdit->text();
    if (query.isEmpty()) return;

    if (query != m_lastSearchQuery) {
        m_lastMatchRow = -1;
        m_lastSearchQuery = query;
        btnSearchStop->setEnabled(true);
        statusLabel->setText("Searching...");
        model->startSearch(query);
        return;
    }

    // Same query — jump to next match
    if (model->matchCount() > 0) {
        int next = model->nextMatch(m_lastMatchRow);
        if (next >= 0) {
            m_lastMatchRow = next;
            scrollToSourceRow(next);
            statusLabel->setText(QString("Match at line %1 | %2 total")
                .arg(next + 1).arg(model->matchCount()));
        }
    }
}

void LogViewerWidget::onSearchStopClicked() {
    model->stopSearch();
    btnSearchStop->setEnabled(false);
    statusLabel->setText("Search interrupted");
}

void LogViewerWidget::onSearchFinished(int matchCount) {
    btnSearchStop->setEnabled(false);

    if (matchCount < 0) {
        statusLabel->setText("Invalid regex pattern");
        m_lastSearchQuery.clear();
        return;
    }

    statusLabel->setText(QString("Matches: %1 / %2 lines")
        .arg(matchCount).arg(model->lineCount()));

    if (matchCount > 0) {
        int first = model->nextMatch(-1);
        if (first >= 0) {
            m_lastMatchRow = first;
            scrollToSourceRow(first);
        }
    }

    if (chkFilterMode->isChecked())
        filterProxy->refreshFilter();
}

void LogViewerWidget::scrollToSourceRow(int sourceRow) {
    QModelIndex srcIdx = model->index(sourceRow);
    QModelIndex proxyIdx = filterProxy->mapFromSource(srcIdx);
    if (proxyIdx.isValid()) {
        listView->setCurrentIndex(proxyIdx);
        listView->scrollTo(proxyIdx, QAbstractItemView::PositionAtCenter);
    }
}

// ─── Filter mode ───────────────────────────────────────────────────────

void LogViewerWidget::onFilterModeToggled(bool checked) {
    filterProxy->setRegexFilterActive(checked);
}

// ─── Timestamps ────────────────────────────────────────────────────────

void LogViewerWidget::onTimestampsDetected(const QDateTime &first,
                                            const QDateTime &last) {
    m_timestampsLoading = true;
    if (first.isValid()) timeStart->setDateTime(first);
    if (last.isValid())  timeEnd->setDateTime(last);
    m_timestampsLoading = false;
}

void LogViewerWidget::onTimeRangeChanged() {
    if (m_timestampsLoading) return;

    QDateTime start = timeStart->dateTime();
    QDateTime end   = timeEnd->dateTime();

    if (start.isValid() && end.isValid() && start < end) {
        filterProxy->setTimeRange(start, end);
        filterProxy->setTimeFilterActive(true);
        statusLabel->setText(QString("Time filter: %1 — %2")
            .arg(start.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(end.toString("yyyy-MM-dd hh:mm:ss")));
    }
}

// ─── Follow / tail ─────────────────────────────────────────────────────

void LogViewerWidget::onFollowToggled(bool checked) {
    model->setFollowEnabled(checked);
    if (checked)
        QTimer::singleShot(0, listView, &QListView::scrollToBottom);
}

void LogViewerWidget::onTailUpdated() {
    statusLabel->setText(QString("Lines: %1 | %2 (following)")
        .arg(model->lineCount()).arg(currentFile));
    if (chkFollow->isChecked())
        QTimer::singleShot(0, listView, &QListView::scrollToBottom);
}

// ─── Copy ──────────────────────────────────────────────────────────────

void LogViewerWidget::copySelectedLines() {
    QModelIndexList selected = listView->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) return;

    std::sort(selected.begin(), selected.end(),
              [](const QModelIndex &a, const QModelIndex &b) {
                  return a.row() < b.row();
              });

    QStringList lines;
    for (const QModelIndex &idx : selected)
        lines << idx.data(Qt::DisplayRole).toString();

    QApplication::clipboard()->setText(lines.join('\n'));
    statusLabel->setText(QString("Copied %1 line(s)").arg(lines.size()));
}
