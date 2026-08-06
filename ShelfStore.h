#ifndef SHELFSTORE_H
#define SHELFSTORE_H

#include <QObject>
#include <QJsonArray>
#include <QString>

// Shared, file-backed store exposed to the QWebChannel "library" pages
// (bookmarks.html / history.html). Reads and writes a JSON array at
// <AppDataLocation>/<fileName> on every mutation, so multiple windows stay
// in sync without extra plumbing.
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

    // Entries older than this many days are dropped on read (0 = keep all).
    void setRetentionDays(int days);

signals:
    void changed();

private:
    QJsonArray loadArray() const;
    void saveArray(const QJsonArray &array) const;

    QString m_fileName;
    int m_retentionDays;
};

#endif
