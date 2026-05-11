#include "LogModel.h"
#include <QDebug>
#include <QColor>
#include <re2/re2.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

// Common timestamp patterns for tryParseTimestamp
// ISO:    2024-05-03T14:23:01  or  2024-05-03 14:23:01
// Syslog: May  3 14:23:01
// Nginx:  03/May/2024:14:23:01

static const char *kReIso =
    R"((\d{4})-(\d{2})-(\d{2})[T ](\d{2}):(\d{2}):(\d{2}))";
static const char *kReSyslog =
    R"((\w{3})\s+(\d{1,2})\s+(\d{2}):(\d{2}):(\d{2}))";
static const char *kReNginx =
    R"((\d{2})/(\w{3})/(\d{4}):(\d{2}):(\d{2}):(\d{2}))";

static int monthFromAbbr(const std::string &m) {
    static const char *months[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    for (int i = 0; i < 12; ++i)
        if (m == months[i]) return i + 1;
    return 1;
}

// ─── Construction / Destruction ────────────────────────────────────────

LogModel::LogModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &LogModel::onFileChanged);
}

LogModel::~LogModel() {
    stopSearch();
    cleanup();
}

void LogModel::cleanup() {
    if (m_watcher && !m_filePath.isEmpty())
        m_watcher->removePath(m_filePath);

    if (m_mappedData && m_mappedData != MAP_FAILED) {
        munmap(const_cast<char*>(m_mappedData), m_mappedSize);
        m_mappedData = nullptr;
        m_mappedSize = 0;
    }
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
    m_lineOffsets.clear();
    m_matches.clear();
    m_totalMatches = 0;
}

// ─── QAbstractListModel overrides ──────────────────────────────────────

int LogModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_lineOffsets.size() > 1
        ? static_cast<int>(m_lineOffsets.size() - 1)
        : 0;
}

int LogModel::lineCount() const { return rowCount(); }

QString LogModel::lineText(int row) const {
    if (row < 0 || row >= lineCount() || !m_mappedData) return {};
    const uint64_t start = m_lineOffsets[row];
    const uint64_t end   = m_lineOffsets[row + 1];
    uint64_t len = end - start;
    while (len > 0 && (m_mappedData[start + len - 1] == '\n' ||
                       m_mappedData[start + len - 1] == '\r'))
        --len;
    return QString::fromUtf8(m_mappedData + start, static_cast<int>(len));
}

QVariant LogModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return {};
    const int row = index.row();
    if (row < 0 || row >= lineCount()) return {};

    if (role == Qt::DisplayRole)
        return lineText(row);

    // Highlight matched lines with a subtle background
    if (role == Qt::BackgroundRole) {
        if (row < (int)m_matches.size() && m_matches[row])
            return QColor(60, 60, 0); // dark yellow
    }

    return {};
}

// ─── File loading ──────────────────────────────────────────────────────

void LogModel::loadFile(const QString& filePath) {
    beginResetModel();
    stopSearch();
    cleanup();

    m_filePath = filePath;
    const QByteArray pathBytes = filePath.toUtf8();

    m_fd = open(pathBytes.constData(), O_RDONLY);
    if (m_fd < 0) {
        qWarning() << "LogModel: open failed:" << filePath;
        endResetModel();
        return;
    }

    struct stat st;
    if (fstat(m_fd, &st) != 0 || st.st_size == 0) {
        qWarning() << "LogModel: empty or stat failed:" << filePath;
        close(m_fd); m_fd = -1;
        endResetModel();
        return;
    }

    m_mappedSize = static_cast<size_t>(st.st_size);
    m_mappedData = static_cast<const char*>(
        mmap(nullptr, m_mappedSize, PROT_READ, MAP_SHARED, m_fd, 0));

    if (m_mappedData == MAP_FAILED) {
        qWarning() << "LogModel: mmap failed:" << filePath;
        m_mappedData = nullptr; m_mappedSize = 0;
        close(m_fd); m_fd = -1;
        endResetModel();
        return;
    }

    madvise(const_cast<char*>(m_mappedData), m_mappedSize, MADV_SEQUENTIAL);

    // Build line-offset index
    m_lineOffsets.reserve(m_mappedSize / 60);
    m_lineOffsets.push_back(0);
    for (size_t i = 0; i < m_mappedSize; ++i) {
        if (m_mappedData[i] == '\n' && i + 1 < m_mappedSize)
            m_lineOffsets.push_back(i + 1);
    }
    m_lineOffsets.push_back(m_mappedSize); // sentinel

    endResetModel();
    qDebug() << "LogModel indexed:" << filePath << "lines:" << lineCount();

    parseTimestamps();

    // Set up file watcher for tail
    m_watcher->addPath(m_filePath);
}

// ─── Search ────────────────────────────────────────────────────────────

void LogModel::startSearch(const QString& query) {
    stopSearch(); // cancel any previous

    if (query.isEmpty() || lineCount() == 0) {
        m_matches.clear();
        m_totalMatches = 0;
        emit dataChanged(index(0), index(lineCount() - 1), {Qt::BackgroundRole});
        emit searchFinished(0);
        return;
    }

    m_matches.assign(lineCount(), false);
    m_totalMatches = 0;

    // Launch background search with jthread + stop_token
    m_searchThread = std::jthread([this, pattern = query.toStdString()]
                                  (std::stop_token stoken) {
        re2::RE2 re(pattern);
        if (!re.ok()) {
            QMetaObject::invokeMethod(this, [this]() {
                emit searchFinished(-1); // -1 = invalid regex
            }, Qt::QueuedConnection);
            return;
        }

        const int total = lineCount();
        int matches = 0;

        for (int i = 0; i < total; ++i) {
            if (stoken.stop_requested()) break;

            const uint64_t start = m_lineOffsets[i];
            const uint64_t end   = m_lineOffsets[i + 1];
            uint64_t len = end - start;
            while (len > 0 && (m_mappedData[start + len - 1] == '\n' ||
                               m_mappedData[start + len - 1] == '\r'))
                --len;

            re2::StringPiece line(m_mappedData + start, len);
            if (re2::RE2::PartialMatch(line, re)) {
                m_matches[i] = true;
                ++matches;
            }
        }

        m_totalMatches = matches;

        // Notify UI from main thread
        QMetaObject::invokeMethod(this, [this, matches]() {
            emit dataChanged(index(0), index(lineCount() - 1), {Qt::BackgroundRole});
            emit searchFinished(matches);
        }, Qt::QueuedConnection);
    });
}

void LogModel::stopSearch() {
    if (m_searchThread.joinable()) {
        m_searchThread.request_stop();
        m_searchThread.join();
    }
}

bool LogModel::isMatch(int row) const {
    if (row < 0 || row >= (int)m_matches.size()) return false;
    return m_matches[row];
}

int LogModel::matchCount() const {
    return m_totalMatches.load();
}

int LogModel::nextMatch(int fromRow) const {
    const int total = lineCount();
    if (total == 0 || m_matches.empty()) return -1;
    for (int i = 1; i <= total; ++i) {
        int idx = (fromRow + i) % total;
        if (m_matches[idx]) return idx;
    }
    return -1;
}

// ─── Timestamp parsing ─────────────────────────────────────────────────

QDateTime LogModel::tryParseTimestamp(const char *data, int len) {
    re2::StringPiece sp(data, len);
    int y, mo, d, h, mi, s;
    std::string ms;

    // ISO 8601
    if (re2::RE2::PartialMatch(sp, kReIso, &y, &mo, &d, &h, &mi, &s))
        return QDateTime(QDate(y, mo, d), QTime(h, mi, s));

    // Nginx / Apache: 03/May/2024:14:23:01
    if (re2::RE2::PartialMatch(sp, kReNginx, &d, &ms, &y, &h, &mi, &s))
        return QDateTime(QDate(y, monthFromAbbr(ms), d), QTime(h, mi, s));

    // Syslog: May  3 14:23:01 (no year — use current)
    if (re2::RE2::PartialMatch(sp, kReSyslog, &ms, &d, &h, &mi, &s))
        return QDateTime(QDate(QDate::currentDate().year(), monthFromAbbr(ms), d),
                         QTime(h, mi, s));

    return {};
}

QDateTime LogModel::parseTimestampFromLine(const QString &line) {
    QByteArray utf8 = line.toUtf8();
    return tryParseTimestamp(utf8.constData(), utf8.size());
}

void LogModel::parseTimestamps() {
    m_firstTimestamp = {};
    m_lastTimestamp  = {};
    if (lineCount() == 0 || !m_mappedData) return;

    // First line
    {
        uint64_t s = m_lineOffsets[0], e = m_lineOffsets[1];
        uint64_t len = e - s;
        while (len > 0 && (m_mappedData[s+len-1]=='\n'||m_mappedData[s+len-1]=='\r')) --len;
        m_firstTimestamp = tryParseTimestamp(m_mappedData + s, (int)len);
    }
    // Last line
    {
        int last = lineCount() - 1;
        uint64_t s = m_lineOffsets[last], e = m_lineOffsets[last + 1];
        uint64_t len = e - s;
        while (len > 0 && (m_mappedData[s+len-1]=='\n'||m_mappedData[s+len-1]=='\r')) --len;
        m_lastTimestamp = tryParseTimestamp(m_mappedData + s, (int)len);
    }

    if (m_firstTimestamp.isValid() || m_lastTimestamp.isValid())
        emit timestampsDetected(m_firstTimestamp, m_lastTimestamp);
}

// ─── Follow / tail ─────────────────────────────────────────────────────

void LogModel::setFollowEnabled(bool enabled) {
    m_followEnabled = enabled;
}

void LogModel::onFileChanged(const QString &path) {
    if (path != m_filePath || !m_followEnabled) return;

    struct stat st;
    if (stat(m_filePath.toUtf8().constData(), &st) != 0) return;
    size_t newSize = static_cast<size_t>(st.st_size);
    if (newSize <= m_mappedSize) return; // file didn't grow

    size_t oldSize = m_mappedSize;

    // Unmap old, remap at new size
    if (m_mappedData && m_mappedData != MAP_FAILED)
        munmap(const_cast<char*>(m_mappedData), m_mappedSize);

    m_mappedSize = newSize;
    m_mappedData = static_cast<const char*>(
        mmap(nullptr, m_mappedSize, PROT_READ, MAP_SHARED, m_fd, 0));

    if (m_mappedData == MAP_FAILED) {
        m_mappedData = nullptr; m_mappedSize = 0;
        return;
    }

    // Scan only the new portion for additional line offsets
    // Remove the old sentinel first
    if (!m_lineOffsets.empty())
        m_lineOffsets.pop_back();

    int oldLineCount = m_lineOffsets.size() > 0
        ? static_cast<int>(m_lineOffsets.size()) - 0
        : 0;

    for (size_t i = oldSize; i < m_mappedSize; ++i) {
        if (m_mappedData[i] == '\n' && i + 1 < m_mappedSize)
            m_lineOffsets.push_back(i + 1);
    }
    m_lineOffsets.push_back(m_mappedSize); // new sentinel

    int newLineCount = static_cast<int>(m_lineOffsets.size()) - 1;
    if (newLineCount > oldLineCount) {
        beginInsertRows(QModelIndex(), oldLineCount, newLineCount - 1);
        endInsertRows();
    }

    emit tailUpdated();

    // QFileSystemWatcher may remove the path after a change; re-add it
    if (!m_watcher->files().contains(m_filePath))
        m_watcher->addPath(m_filePath);
}
