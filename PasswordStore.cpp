#include "PasswordStore.h"
#include "VaultCrypto.h"
#include "OSPaths.h"
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

// Normalize an origin to scheme+host+port (e.g., "https://example.com:443").
// Strips path, query, fragment. Does NOT strip www. or default ports.
QString normOrigin(const QString &urlOrOrigin)
{
    QString input = urlOrOrigin.trimmed();
    if (input.isEmpty())
        return QString();

    QUrl url(input);
    // If the input doesn't parse as a valid URL with scheme, assume it's just a host
    if (!url.isValid() || url.scheme().isEmpty()) {
        // Try with https:// prefix
        url = QUrl(QStringLiteral("https://") + input);
    }
    if (!url.isValid() || url.host().isEmpty())
        return QString();

    QString origin = url.scheme().toLower() + QStringLiteral("://") + url.host().toLower();
    int port = url.port();
    // Include port only if non-standard (not 80 for http, not 443 for https)
    if (port != -1 && !((port == 80 && url.scheme() == QLatin1String("http")) ||
                        (port == 443 && url.scheme() == QLatin1String("https")))) {
        origin += QLatin1Char(':') + QString::number(port);
    }
    return origin;
}

// Legacy host normalization (for never-save list which is host-based)
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
        const QString origin = normOrigin(o.value(QStringLiteral("host")).toString());
        if (origin.isEmpty() || seen.contains(origin))
            continue;
        seen.insert(origin);
        QVariantMap m;
        m[QStringLiteral("origin")] = origin;
        m[QStringLiteral("username")] = o.value(QStringLiteral("username")).toString();
        m[QStringLiteral("timestamp")] = o.value(QStringLiteral("timestamp")).toString();
        result.append(m);
    }
    return result;
}

QString PasswordStore::passwordFor(const QString &origin, const QString &username) const
{
    const QString o = normOrigin(origin);
    if (o.isEmpty())
        return QString();
    const QJsonArray array = loadArray();
    for (const QJsonValue &value : array) {
        const QJsonObject obj = value.toObject();
        if (normOrigin(obj.value(QStringLiteral("host")).toString()) == o
            && obj.value(QStringLiteral("username")).toString() == username)
            return obj.value(QStringLiteral("password")).toString();
    }
    return QString();
}

void PasswordStore::save(const QString &origin, const QString &username, const QString &password)
{
    const QString o = normOrigin(origin);
    if (o.isEmpty() || password.isEmpty())
        return;
    QJsonArray array = loadArray();
    for (int i = array.size() - 1; i >= 0; --i) {
        const QJsonObject obj = array.at(i).toObject();
        if (normOrigin(obj.value(QStringLiteral("host")).toString()) == o
            && obj.value(QStringLiteral("username")).toString() == username)
            array.removeAt(i);
    }
    QJsonObject item;
    item[QStringLiteral("host")] = o;
    item[QStringLiteral("username")] = username;
    item[QStringLiteral("password")] = password;
    item[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString(Qt::ISODate);
    array.prepend(item);
    saveArray(array);
    emit changed();
}

void PasswordStore::remove(const QString &origin, const QString &username)
{
    const QString o = normOrigin(origin);
    if (o.isEmpty())
        return;
    QJsonArray array = loadArray();
    for (int i = array.size() - 1; i >= 0; --i) {
        const QJsonObject obj = array.at(i).toObject();
        if (normOrigin(obj.value(QStringLiteral("host")).toString()) == o
            && (username.isEmpty()
                || obj.value(QStringLiteral("username")).toString() == username))
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

QVariantList PasswordStore::entriesFor(const QString &origin) const
{
    const QString o = normOrigin(origin);
    if (o.isEmpty())
        return {};
    QVariantList result;
    const QJsonArray array = loadArray();
    for (const QJsonValue &value : array) {
        const QJsonObject obj = value.toObject();
        if (normOrigin(obj.value(QStringLiteral("host")).toString()) == o) {
            QVariantMap m;
            m[QStringLiteral("username")] = obj.value(QStringLiteral("username")).toString();
            m[QStringLiteral("password")] = obj.value(QStringLiteral("password")).toString();
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
    OSPaths::writeFileAtomic(neverSaveFile(), QJsonDocument(array).toJson());
    emit changed();
}

QJsonArray PasswordStore::loadArray() const
{
    QFile file(pwdFile());
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray data = file.readAll();
        if (VaultCrypto::isEnvelope(data)) {
            const QByteArray plain = VaultCrypto::decrypt(data);
            if (plain.isEmpty()) {
                // The vault exists but failed authentication (tampered, or the
                // master key changed). Never collapse this into an "empty
                // vault": saveArray() refuses to write while this flag is set,
                // so a subsequent save cannot overwrite real encrypted data.
                m_authFailed = true;
                return QJsonArray();
            }
            m_authFailed = false;
            const QJsonDocument doc = QJsonDocument::fromJson(plain);
            if (doc.isArray())
                return doc.array();
        } else {
            // Legacy file (previous builds stored a plaintext JSON array).
            // Load it so existing data is not silently lost; it is re-encrypted
            // on the next save. Entries written with the old XOR cipher will
            // read back as garbage and need re-entering.
            m_authFailed = false;
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isArray())
                return doc.array();
        }
    }
    m_authFailed = false;
    return QJsonArray();
}

void PasswordStore::saveArray(const QJsonArray &array) const
{
    if (m_authFailed)
        return; // never overwrite a vault we failed to authenticate
    const QByteArray plain = QJsonDocument(array).toJson();
    const QByteArray envelope = VaultCrypto::encrypt(plain);
    if (envelope.isEmpty())
        return;
    OSPaths::writeFileAtomic(pwdFile(), envelope);
}
