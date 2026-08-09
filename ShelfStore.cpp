#include "ShelfStore.h"
#include "OSPaths.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QUrl>
#include <QSet>

namespace {
const int kMaxEntries = 500;

// Canonical bookmark URL: trimmed, hostname lowercased, trailing "/" stripped
// everywhere except on the bare root (so https://example.com/ and
// https://example.com/path/ are dedup-friendly while the root keeps its form).
QString normalizedUrl(const QString &input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty())
        return QString();
    QUrl url(trimmed);
    if (url.scheme().isEmpty() && url.host().isEmpty())
        url = QUrl(QStringLiteral("https://") + trimmed);
    if (!url.isValid() || url.host().isEmpty())
        return QString();
    QString result = url.scheme().toLower() + QStringLiteral("://") + url.host().toLower();
    QString path = url.path();
    while (path.size() > 1 && path.endsWith(QLatin1Char('/')))
        path.chop(1);
    result += path;
    if (!url.query().isEmpty())
        result += QStringLiteral("?") + url.query();
    if (!url.fragment().isEmpty())
        result += QStringLiteral("#") + url.fragment();
    return result;
}
}

ShelfStore::ShelfStore(const QString &fileName, QObject *parent)
    : QObject(parent)
    , m_fileName(fileName)
    , m_retentionDays(0)
{
}

QString ShelfStore::json() const
{
    // Pure read: pruning/persistence happens inside loadArray(), so this
    // getter (called on every URL-suggestion keystroke) never writes to disk.
    return QJsonDocument(loadArray()).toJson(QJsonDocument::Compact);
}

void ShelfStore::add(const QString &title, const QString &url)
{
    if (url.isEmpty())
        return;
    QJsonArray array = loadArray();
    for (int i = array.size() - 1; i >= 0; --i) {
        if (array.at(i).toObject().value(QStringLiteral("url")).toString() == url)
            array.removeAt(i);
    }
    QJsonObject item;
    item[QStringLiteral("title")] = title.isEmpty() ? url : title;
    item[QStringLiteral("url")] = url;
    item[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    array.prepend(item);
    while (array.size() > kMaxEntries)
        array.removeLast();
    saveArray(array);
    emit changed();
}

void ShelfStore::remove(const QString &url)
{
    if (url.isEmpty())
        return;
    QJsonArray array = loadArray();
    for (int i = array.size() - 1; i >= 0; --i) {
        if (array.at(i).toObject().value(QStringLiteral("url")).toString() == url)
            array.removeAt(i);
    }
    saveArray(array);
    emit changed();
}

void ShelfStore::clearAll()
{
    saveArray(QJsonArray());
    emit changed();
}

int ShelfStore::importBookmarks(const QJsonArray &items)
{
    // Single read: pull the current store once and merge in memory.
    QJsonArray array = loadArray();

    // Existing entries are matched by both their raw and normalized forms so
    // previously-added unnormalized URLs still dedupe correctly.
    QSet<QString> seen;
    for (const QJsonValue &value : array) {
        const QString raw = value.toObject().value(QStringLiteral("url")).toString();
        if (raw.isEmpty())
            continue;
        seen.insert(raw);
        const QString norm = normalizedUrl(raw);
        if (!norm.isEmpty())
            seen.insert(norm);
    }

    // Dedupe the incoming batch against itself and the store.
    QJsonArray additions;
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);
    for (const QJsonValue &value : items) {
        const QJsonObject obj = value.toObject();
        const QString url = normalizedUrl(obj.value(QStringLiteral("url")).toString());
        if (url.isEmpty() || seen.contains(url))
            continue;
        seen.insert(url);
        QString title = obj.value(QStringLiteral("title")).toString().trimmed();
        if (title.isEmpty())
            title = url;
        QJsonObject item;
        item[QStringLiteral("title")] = title;
        item[QStringLiteral("url")] = url;
        item[QStringLiteral("timestamp")] = now;
        additions.append(item);
    }

    if (additions.isEmpty())
        return 0;

    // Newest first: prepend the batch (in order) so the first imported item is
    // the most recent. Bulk import may temporarily exceed kMaxEntries; the cap
    // is only enforced at the end, keeping the newest entries.
    for (int i = additions.size() - 1; i >= 0; --i)
        array.prepend(additions.at(i));
    while (array.size() > kMaxEntries)
        array.removeLast();

    // Single bulk write for the whole import.
    saveArray(array);
    emit changed();
    return additions.size();
}

void ShelfStore::setRetentionDays(int days)
{
    if (m_retentionDays != days) {
        m_retentionDays = days;
        emit changed();
    }
}

QJsonArray ShelfStore::loadArray() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QFile file(dir + QLatin1Char('/') + m_fileName);
    if (!file.open(QIODevice::ReadOnly))
        return QJsonArray();
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray())
        return QJsonArray();
    QJsonArray array = doc.array();

    // Enforce the retention policy here so reads stay correct without making
    // the json() getter persist on every call. A write only happens when
    // expired entries are actually dropped (once, not per keystroke).
    if (m_retentionDays > 0) {
        const QDateTime cutoff = QDateTime::currentDateTime().addDays(-m_retentionDays);
        QJsonArray kept;
        for (const QJsonValue &value : array) {
            const QString ts = value.toObject().value(QStringLiteral("timestamp")).toString();
            if (ts.isEmpty() || QDateTime::fromString(ts, Qt::ISODate) >= cutoff)
                kept.append(value);
        }
        if (kept.size() != array.size()) {
            array = kept;
            saveArray(array);
        }
    }
    return array;
}

void ShelfStore::saveArray(const QJsonArray &array) const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    OSPaths::writeFileAtomic(dir + QLatin1Char('/') + m_fileName,
                             QJsonDocument(array).toJson());
}
