#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QString>
#include <vector>
#include <cstdint>
#include <thread>
#include <atomic>

class LogModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit LogModel(QObject *parent = nullptr);
    ~LogModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void loadFile(const QString& filePath);

    // Search — returns match count
    void startSearch(const QString& query);
    void stopSearch();
    bool isMatch(int row) const;
    int  matchCount() const;
    int  nextMatch(int fromRow) const;

    // Line access
    QString lineText(int row) const;
    int lineCount() const;

    // Timestamps detected from first/last lines
    QDateTime firstTimestamp() const { return m_firstTimestamp; }
    QDateTime lastTimestamp()  const { return m_lastTimestamp;  }

    // Follow / tail
    void setFollowEnabled(bool enabled);

    // Timestamp parsing for external use (filter proxy)
    static QDateTime parseTimestampFromLine(const QString &line);

signals:
    void searchFinished(int matchCount);
    void timestampsDetected(const QDateTime &first, const QDateTime &last);
    void tailUpdated();

private slots:
    void onFileChanged(const QString &path);

private:
    void cleanup();
    void parseTimestamps();
    static QDateTime tryParseTimestamp(const char *data, int len);

    QString m_filePath;

    // mmap state
    const char *m_mappedData = nullptr;
    size_t m_mappedSize = 0;
    int m_fd = -1;

    // Line offset index (sentinel at end = file size)
    std::vector<uint64_t> m_lineOffsets;

    // Search state
    std::vector<bool> m_matches;
    std::jthread m_searchThread;
    std::atomic<int> m_totalMatches{0};

    // Timestamps
    QDateTime m_firstTimestamp;
    QDateTime m_lastTimestamp;

    // File watching
    QFileSystemWatcher *m_watcher = nullptr;
    bool m_followEnabled = false;
};
