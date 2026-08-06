#include "PasswordStore.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QByteArray>

namespace {
QByteArray xorKey() { return QByteArray::fromHex("4a6f5e7d8c2b9a41e0d3c6f2a18b5576"); }

QString pwdFile()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("passwords.json");
}
}

PasswordStore::PasswordStore(QObject *parent)
    : QObject(parent)
{
}

QString PasswordStore::json() const
{
    QJsonArray array = loadArray();
    for (int i = 0; i < array.size(); ++i) {
        QJsonObject o = array.at(i).toObject();
        o[QStringLiteral("password")] = deobfuscate(o.value(QStringLiteral("password")).toString());
        array[i] = o;
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void PasswordStore::save(const QString &host, const QString &username, const QString &password)
{
    if (host.isEmpty() || password.isEmpty())
        return;
    QJsonArray array = loadArray();
    for (int i = array.size() - 1; i >= 0; --i) {
        const QJsonObject o = array.at(i).toObject();
        if (o.value(QStringLiteral("host")).toString() == host
            && o.value(QStringLiteral("username")).toString() == username)
            array.removeAt(i);
    }
    QJsonObject item;
    item[QStringLiteral("host")] = host;
    item[QStringLiteral("username")] = username;
    item[QStringLiteral("password")] = obfuscate(password);
    item[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    array.prepend(item);
    saveArray(array);
    emit changed();
}

void PasswordStore::remove(const QString &host, const QString &username)
{
    if (host.isEmpty())
        return;
    QJsonArray array = loadArray();
    for (int i = array.size() - 1; i >= 0; --i) {
        const QJsonObject o = array.at(i).toObject();
        if (o.value(QStringLiteral("host")).toString() == host
            && (username.isEmpty()
                || o.value(QStringLiteral("username")).toString() == username))
            array.removeAt(i);
    }
    saveArray(array);
    emit changed();
}

void PasswordStore::clearAll()
{
    saveArray(QJsonArray());
    emit changed();
}

QVariantList PasswordStore::entriesFor(const QString &host) const
{
    if (host.isEmpty())
        return {};
    QVariantList result;
    const QJsonArray array = loadArray();
    for (const QJsonValue &value : array) {
        const QJsonObject o = value.toObject();
        if (o.value(QStringLiteral("host")).toString() == host) {
            QVariantMap m;
            m[QStringLiteral("username")] = o.value(QStringLiteral("username")).toString();
            m[QStringLiteral("password")] = deobfuscate(o.value(QStringLiteral("password")).toString());
            result.append(m);
        }
    }
    return result;
}

QJsonArray PasswordStore::loadArray() const
{
    QFile file(pwdFile());
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray())
            return doc.array();
    }
    return QJsonArray();
}

void PasswordStore::saveArray(const QJsonArray &array) const
{
    QFile file(pwdFile());
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(array).toJson());
}

QString PasswordStore::obfuscate(const QString &s) const
{
    const QByteArray key = xorKey();
    const QByteArray in = s.toUtf8();
    QByteArray out(in.size(), Qt::Uninitialized);
    for (int i = 0; i < in.size(); ++i)
        out[i] = in.at(i) ^ key.at(i % key.size());
    return QString::fromLatin1(out.toBase64(QByteArray::Base64Encoding));
}

QString PasswordStore::deobfuscate(const QString &s) const
{
    const QByteArray key = xorKey();
    const QByteArray in = QByteArray::fromBase64(s.toLatin1());
    QByteArray out(in.size(), Qt::Uninitialized);
    for (int i = 0; i < in.size(); ++i)
        out[i] = in.at(i) ^ key.at(i % key.size());
    return QString::fromUtf8(out);
}
