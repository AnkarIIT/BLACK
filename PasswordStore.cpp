#include "PasswordStore.h"
#include "VaultCrypto.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QByteArray>
#include <QSet>

namespace {
QString pwdFile()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("passwords.json");
}

QString neverSaveFile()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + QStringLiteral("never_save.json");
}

QString normHost(const QString &host)
{
    QString h = host.trimmed().toLower();
    if (!h.startsWith(QLatin1Char('['))) {   // not a bracketed IPv6 literal
        const int colon = h.indexOf(QLatin1Char(':'));
        if (colon > 0)
            h.truncate(colon);               // drop any :port
    }
    if (h.startsWith(QStringLiteral("www.")))
        h.remove(0, 4);
    return h;
}
}

PasswordStore::PasswordStore(QObject *parent)
    : QObject(parent)
{
}

QVariantList PasswordStore::hosts() const
{
    QVariantList result;
    QSet<QString> seen;
    const QJsonArray array = loadArray();
    for (const QJsonValue &value : array) {
        const QJsonObject o = value.toObject();
        const QString host = normHost(o.value(QStringLiteral("host")).toString());
        if (host.isEmpty() || seen.contains(host))
            continue;
        seen.insert(host);
        QVariantMap m;
        m[QStringLiteral("host")] = host;
        m[QStringLiteral("username")] = o.value(QStringLiteral("username")).toString();
        m[QStringLiteral("timestamp")] = o.value(QStringLiteral("timestamp")).toString();
        result.append(m);
    }
    return result;
}

QString PasswordStore::passwordFor(const QString &host, const QString &username) const
{
    const QString h = normHost(host);
    if (h.isEmpty())
        return QString();
    const QJsonArray array = loadArray();
    for (const QJsonValue &value : array) {
        const QJsonObject o = value.toObject();
        if (normHost(o.value(QStringLiteral("host")).toString()) == h
            && o.value(QStringLiteral("username")).toString() == username)
            return o.value(QStringLiteral("password")).toString();
    }
    return QString();
}

void PasswordStore::save(const QString &host, const QString &username, const QString &password)
{
    const QString h = normHost(host);
    if (h.isEmpty() || password.isEmpty())
        return;
    QJsonArray array = loadArray();
    for (int i = array.size() - 1; i >= 0; --i) {
        const QJsonObject o = array.at(i).toObject();
        if (normHost(o.value(QStringLiteral("host")).toString()) == h
            && o.value(QStringLiteral("username")).toString() == username)
            array.removeAt(i);
    }
    QJsonObject item;
    item[QStringLiteral("host")] = h;
    item[QStringLiteral("username")] = username;
    item[QStringLiteral("password")] = password;
    item[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    array.prepend(item);
    saveArray(array);
    emit changed();
}

void PasswordStore::remove(const QString &host, const QString &username)
{
    const QString h = normHost(host);
    if (h.isEmpty())
        return;
    QJsonArray array = loadArray();
    for (int i = array.size() - 1; i >= 0; --i) {
        const QJsonObject o = array.at(i).toObject();
        if (normHost(o.value(QStringLiteral("host")).toString()) == h
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
    const QString h = normHost(host);
    if (h.isEmpty())
        return {};
    QVariantList result;
    const QJsonArray array = loadArray();
    for (const QJsonValue &value : array) {
        const QJsonObject o = value.toObject();
        if (normHost(o.value(QStringLiteral("host")).toString()) == h) {
            QVariantMap m;
            m[QStringLiteral("username")] = o.value(QStringLiteral("username")).toString();
            m[QStringLiteral("password")] = o.value(QStringLiteral("password")).toString();
            result.append(m);
        }
    }
    return result;
}

QString PasswordStore::neverSaveJson() const
{
    QFile file(neverSaveFile());
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray())
            return QString::fromUtf8(QJsonDocument(doc.array()).toJson(QJsonDocument::Compact));
    }
    return QStringLiteral("[]");
}

bool PasswordStore::isNeverSave(const QString &host) const
{
    const QString h = normHost(host);
    if (h.isEmpty())
        return false;
    QFile file(neverSaveFile());
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) {
            const QJsonArray array = doc.array();
            for (const QJsonValue &v : array) {
                if (v.toString() == h)
                    return true;
            }
        }
    }
    return false;
}

void PasswordStore::setNeverSave(const QString &host, bool neverSave)
{
    const QString h = normHost(host);
    if (h.isEmpty())
        return;
    QJsonArray array;
    QFile file(neverSaveFile());
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray())
            array = doc.array();
    }
    if (neverSave) {
        for (const QJsonValue &v : array) {
            if (v.toString() == h) {
                emit changed();
                return;
            }
        }
        array.append(h);
    } else {
        for (int i = array.size() - 1; i >= 0; --i) {
            if (array.at(i).toString() == h)
                array.removeAt(i);
        }
    }
    QFile out(neverSaveFile());
    if (out.open(QIODevice::WriteOnly))
        out.write(QJsonDocument(array).toJson());
    emit changed();
}

QJsonArray PasswordStore::loadArray() const
{
    QFile file(pwdFile());
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray data = file.readAll();
        if (VaultCrypto::isEnvelope(data)) {
            const QByteArray plain = VaultCrypto::decrypt(data);
            if (plain.isEmpty())
                return QJsonArray(); // failed authentication: treat as empty
            const QJsonDocument doc = QJsonDocument::fromJson(plain);
            if (doc.isArray())
                return doc.array();
        } else {
            // Legacy file (previous builds stored a plaintext JSON array).
            // Load it so existing data is not silently lost; it is re-encrypted
            // on the next save. Entries written with the old XOR cipher will
            // read back as garbage and need re-entering.
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isArray())
                return doc.array();
        }
    }
    return QJsonArray();
}

void PasswordStore::saveArray(const QJsonArray &array) const
{
    const QByteArray plain = QJsonDocument(array).toJson();
    const QByteArray envelope = VaultCrypto::encrypt(plain);
    if (envelope.isEmpty())
        return;
    QFile file(pwdFile());
    if (file.open(QIODevice::WriteOnly))
        file.write(envelope);
}
