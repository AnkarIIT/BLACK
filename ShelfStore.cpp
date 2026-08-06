#include "ShelfStore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

namespace {
const int kMaxEntries = 500;
}

ShelfStore::ShelfStore(const QString &fileName, QObject *parent)
    : QObject(parent)
    , m_fileName(fileName)
    , m_retentionDays(0)
{
}

QString ShelfStore::json() const
{
    QJsonArray array = loadArray();
    bool pruned = false;
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
            pruned = true;
        }
    }
    if (pruned)
        saveArray(array);
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
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
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray())
            return doc.array();
    }
    return QJsonArray();
}

void ShelfStore::saveArray(const QJsonArray &array) const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QFile file(dir + QLatin1Char('/') + m_fileName);
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(array).toJson());
}
