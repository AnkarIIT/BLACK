#include "PermissionsBridge.h"
#include "VaultCrypto.h"
#include "OSPaths.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

PermissionsBridge::PermissionsBridge(QObject *parent)
    : QObject(parent)
{
}

QString PermissionsBridge::keyFor(const QString &origin, int type) const
{
    return origin + QLatin1Char('|') + QString::number(type);
}

QString PermissionsBridge::typeName(int type) const
{
    switch (type) {
    case 0: return QStringLiteral("Notifications");
    case 1: return QStringLiteral("Geolocation");
    case 2: return QStringLiteral("MediaAudioCapture");
    case 3: return QStringLiteral("MediaVideoCapture");
    case 4: return QStringLiteral("MediaAudioVideoCapture");
    case 5: return QStringLiteral("MouseLock");
    case 6: return QStringLiteral("DesktopVideoCapture");
    case 7: return QStringLiteral("DesktopAudioVideoCapture");
    case 8: return QStringLiteral("ClipboardReadWrite");
    case 9: return QStringLiteral("LocalFontsAccess");
    default: return QStringLiteral("Unknown");
    }
}

QJsonArray PermissionsBridge::getPermissions()
{
    QJsonArray result;
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QLatin1Char('/') + QStringLiteral("permissions.json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return result;

    const QByteArray raw = file.readAll();
    const QByteArray plain = VaultCrypto::decrypt(raw);
    const QByteArray payload = plain.isEmpty() ? raw : plain;
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return result;

    const QJsonObject obj = doc.object();
    QJsonObject byOrigin;
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QString key = it.key();
        const bool allowed = it.value().toBool();
        const int sep = key.lastIndexOf(QLatin1Char('|'));
        if (sep <= 0)
            continue;
        const QString origin = key.left(sep);
        const int type = key.mid(sep + 1).toInt();
        if (!byOrigin.contains(origin))
            byOrigin[origin] = QJsonArray();
        QJsonObject perm;
        perm[QStringLiteral("type")] = type;
        perm[QStringLiteral("typeName")] = typeName(type);
        perm[QStringLiteral("allowed")] = allowed;
        byOrigin[origin].toArray().append(perm);
    }

    for (auto it = byOrigin.constBegin(); it != byOrigin.constEnd(); ++it) {
        QJsonObject entry;
        entry[QStringLiteral("origin")] = it.key();
        entry[QStringLiteral("permissions")] = it.value().toArray();
        result.append(entry);
    }
    return result;
}

void PermissionsBridge::removePermission(const QString &origin, int type)
{
    const QString key = keyFor(origin, type);
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QLatin1Char('/') + QStringLiteral("permissions.json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QByteArray raw = file.readAll();
    const QByteArray plain = VaultCrypto::decrypt(raw);
    const QByteArray payload = plain.isEmpty() ? raw : plain;
    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return;
    QJsonObject obj = doc.object();
    obj.remove(key);
    const QByteArray newPayload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    const QByteArray blob = VaultCrypto::encrypt(newPayload);
    if (blob.isEmpty())
        return;
    OSPaths::writeFileAtomic(path, blob);
    emit changed();
}

void PermissionsBridge::clearPermissions()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QLatin1Char('/') + QStringLiteral("permissions.json");
    QFile::remove(path);
    emit changed();
}