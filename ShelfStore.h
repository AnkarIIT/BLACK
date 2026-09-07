#ifndef SHELFSTORE_H
#define SHELFSTORE_H

#include <QObject>
#include <QJsonArray>
#include <QString>

// Shared, file-backed store exposed to the QWebChannel "library" pages
// (bookmarks.html / history.html). Reads and writes an ENCRYPTED JSON array at
// <AppDataLocation>/<fileName> on every mutation, so multiple windows stay
// in sync without extra plumbing. Data is encrypted at rest using VaultCrypto.
class ShelfStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString json READ json NOTIFY changed)

public:
    explicit ShelfStore(const QString &fileName, QObject *parent = nullptr);

    QString json() const;

    Q_INVOKABLE void add(const QString &title, const QString &url);
    Q_INVOKABLE void remove(const QString &url);
    Q_INVOKABLE void clearAll();

    // Bulk-imports a batch of { "title", "url" } entries (e.g. from another
    // browser). URLs are normalized, the batch is deduplicated against itself
    // and against existing entries, and everything is persisted with a single
    // disk write. Returns the number of items actually imported.
    Q_INVOKABLE int importBookmarks(const QJsonArray &items);

    // Entries older than this many days are dropped on read (0 = keep all).
    void setRetentionDays(int days);

    // Normalize a URL for consistent storage and comparison
    static QString normalizedUrl(const QString &input);

    // Prune entries older than retentionDays (if > 0)
    void pruneArray(QJsonArray &array) const;

signals:
    void changed();

private:
    friend class TestShelfStore;
    QJsonArray loadArray() const;
    void saveArray(const QJsonArray &array) const;

    QString m_fileName;
    int m_retentionDays;
};

#endif
