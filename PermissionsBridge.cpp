#include "PermissionsBridge.h"

#include "VaultCrypto.h"
#include "OSPaths.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

PermissionsBridge::PermissionsBridge(QObject *parent)
    : QObject(parent)
{
}

QString PermissionsBridge::keyFor(const QString &origin, int type) const
{
    return origin + QLatin1Char('|') + QString::number(type);
}

QString PermissionsBridge::defaultKeyFor(int type) const
{
    return QStringLiteral("__default__|") + QString::number(type);
}

QString PermissionsBridge::typeName(int type) const
{
    switch (type) {
    case Notifications: return QStringLiteral("Notifications");
    case Geolocation: return QStringLiteral("Location");
    case MediaAudioCapture: return QStringLiteral("Microphone");
    case MediaVideoCapture: return QStringLiteral("Camera");
    case MediaAudioVideoCapture: return QStringLiteral("Camera & Microphone");
    case MouseLock: return QStringLiteral("Pointer Lock");
    case DesktopVideoCapture: return QStringLiteral("Screen Capture (Video)");
    case DesktopAudioVideoCapture: return QStringLiteral("Screen Capture (Audio+Video)");
    case ClipboardReadWrite: return QStringLiteral("Clipboard");
    case LocalFontsAccess: return QStringLiteral("Local Fonts");

    case Cookies: return QStringLiteral("Cookies & Site Data");
    case JavaScript: return QStringLiteral("JavaScript");
    case Images: return QStringLiteral("Images");
    case Popups: return QStringLiteral("Popups & Redirects");
    case AutoPlay: return QStringLiteral("Auto-play");
    case MixedContent: return QStringLiteral("Insecure Content");
    case HttpsUpgrade: return QStringLiteral("HTTPS Upgrade");
    case MidiSysex: return QStringLiteral("MIDI Devices");
    case UsbDevices: return QStringLiteral("USB Devices");
    case HidDevices: return QStringLiteral("HID Devices");
    case SerialPorts: return QStringLiteral("Serial Ports");
    case WindowPlacement: return QStringLiteral("Window Placement");
    case PaymentHandler: return QStringLiteral("Payment Handler");
    case IdleDetection: return QStringLiteral("Idle Detection");
    case BluetoothDevices: return QStringLiteral("Bluetooth Devices");
    case FileSystemAccess: return QStringLiteral("File System Access");
    case StorageAccess: return QStringLiteral("Storage Access");
    case TopLevelStorageAccess: return QStringLiteral("Top-level Storage Access");

    default: return QStringLiteral("Unknown");
    }
}

QString PermissionsBridge::categoryName(int type) const
{
    // Group permissions into UI categories
    if (type <= LocalFontsAccess)
        return QStringLiteral("Permissions");
    if (type <= TopLevelStorageAccess)
        return QStringLiteral("Content Settings");
    return QStringLiteral("Other");
}

int PermissionsBridge::categoryForType(int type) const
{
    if (type <= LocalFontsAccess)
        return 0; // Permissions
    return 1; // Content Settings
}

bool PermissionsBridge::isContentSetting(int type) const
{
    return type >= Cookies && type <= TopLevelStorageAccess;
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
        const int state = it.value().toInt();
        const int sep = key.lastIndexOf(QLatin1Char('|'));
        if (sep <= 0)
            continue;
        const QString origin = key.left(sep);
        const int type = key.mid(sep + 1).toInt();
        // Skip default entries
        if (origin == QStringLiteral("__default__"))
            continue;
        if (!byOrigin.contains(origin))
            byOrigin[origin] = QJsonArray();
        QJsonObject perm;
        perm[QStringLiteral("type")] = type;
        perm[QStringLiteral("typeName")] = typeName(type);
        perm[QStringLiteral("category")] = categoryName(type);
        perm[QStringLiteral("state")] = state;
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

QJsonObject PermissionsBridge::getPermissionsForOrigin(const QString &origin)
{
    QJsonObject result;
    result[QStringLiteral("origin")] = origin;
    QJsonArray permissions;

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
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QString key = it.key();
        const int state = it.value().toInt();
        const int sep = key.lastIndexOf(QLatin1Char('|'));
        if (sep <= 0)
            continue;
        const QString o = key.left(sep);
        const int type = key.mid(sep + 1).toInt();
        if (o != origin || o == QStringLiteral("__default__"))
            continue;
        QJsonObject perm;
        perm[QStringLiteral("type")] = type;
        perm[QStringLiteral("typeName")] = typeName(type);
        perm[QStringLiteral("category")] = categoryName(type);
        perm[QStringLiteral("state")] = state;
        permissions.append(perm);
    }
    result[QStringLiteral("permissions")] = permissions;
    return result;
}

void PermissionsBridge::setPermission(const QString &origin, int type, int state)
{
    if (origin.isEmpty() || type < 0 || type > MaxPermissionType || state < 0 || state > 3)
        return;

    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QLatin1Char('/') + QStringLiteral("permissions.json");
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    QJsonObject obj;
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray raw = file.readAll();
        const QByteArray plain = VaultCrypto::decrypt(raw);
        const QByteArray payload = plain.isEmpty() ? raw : plain;
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isObject())
            obj = doc.object();
    }

    const QString key = keyFor(origin, type);
    if (state == Default) {
        obj.remove(key);
    } else {
        obj[key] = state;
    }

    const QByteArray newPayload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    const QByteArray blob = VaultCrypto::encrypt(newPayload);
    if (blob.isEmpty())
        return;
    if (!OSPaths::writeFileAtomic(path, blob)) {
        qWarning() << "Failed to save permissions";
        return;
    }
    emit changed();
}

void PermissionsBridge::removePermission(const QString &origin, int type)
{
    setPermission(origin, type, Default);
}

void PermissionsBridge::clearPermissions()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QLatin1Char('/') + QStringLiteral("permissions.json");
    QFile::remove(path);
    emit changed();
}

QJsonArray PermissionsBridge::getPermissionTypes()
{
    QJsonArray result;
    for (int type = 0; type <= MaxPermissionType; ++type) {
        if (type <= LocalFontsAccess || isContentSetting(type)) {
            QJsonObject obj;
            obj[QStringLiteral("type")] = type;
            obj[QStringLiteral("name")] = typeName(type);
            obj[QStringLiteral("category")] = categoryName(type);
            obj[QStringLiteral("isContentSetting")] = isContentSetting(type);
            // Default states
            if (type == Cookies || type == Images || type == AutoPlay)
                obj[QStringLiteral("defaultState")] = Allow;
            else if (type == JavaScript)
                obj[QStringLiteral("defaultState")] = Allow;
            else if (type == Popups)
                obj[QStringLiteral("defaultState")] = Block;
            else
                obj[QStringLiteral("defaultState")] = Ask;
            result.append(obj);
        }
    }
    return result;
}

QJsonArray PermissionsBridge::getContentSettingCategories()
{
    QJsonArray result;

    // Permissions category
    QJsonObject permsCat;
    permsCat[QStringLiteral("name")] = QStringLiteral("Permissions");
    permsCat[QStringLiteral("types")] = QJsonArray();
    QJsonArray permTypes;
    for (int type = 0; type <= LocalFontsAccess; ++type) {
        QJsonObject obj;
        obj[QStringLiteral("type")] = type;
        obj[QStringLiteral("name")] = typeName(type);
        permTypes.append(obj);
    }
    permsCat[QStringLiteral("types")] = permTypes;
    result.append(permsCat);

    // Content Settings category
    QJsonObject contentCat;
    contentCat[QStringLiteral("name")] = QStringLiteral("Content Settings");
    QJsonArray contentTypes;
    for (int type = Cookies; type <= TopLevelStorageAccess; ++type) {
        QJsonObject obj;
        obj[QStringLiteral("type")] = type;
        obj[QStringLiteral("name")] = typeName(type);
        contentTypes.append(obj);
    }
    contentCat[QStringLiteral("types")] = contentTypes;
    result.append(contentCat);

    return result;
}

int PermissionsBridge::getDefaultState(int type)
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QLatin1Char('/') + QStringLiteral("permissions.json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return (type == Cookies || type == Images || type == JavaScript) ? Allow : Ask;

    const QByteArray raw = file.readAll();
    const QByteArray plain = VaultCrypto::decrypt(raw);
    const QByteArray payload = plain.isEmpty() ? raw : plain;
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject())
        return (type == Cookies || type == Images || type == JavaScript) ? Allow : Ask;

    const QJsonObject obj = doc.object();
    const QString key = defaultKeyFor(type);
    if (obj.contains(key))
        return obj[key].toInt();

    return (type == Cookies || type == Images || type == JavaScript) ? Allow : Ask;
}

void PermissionsBridge::setDefaultState(int type, int state)
{
    if (type < 0 || type > MaxPermissionType || state < 0 || state > 3)
        return;

    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QLatin1Char('/') + QStringLiteral("permissions.json");
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    QJsonObject obj;
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray raw = file.readAll();
        const QByteArray plain = VaultCrypto::decrypt(raw);
        const QByteArray payload = plain.isEmpty() ? raw : plain;
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        if (doc.isObject())
            obj = doc.object();
    }

    const QString key = defaultKeyFor(type);
    if (state == Ask && type != Cookies && type != Images && type != JavaScript) {
        obj.remove(key);
    } else {
        obj[key] = state;
    }

    const QByteArray newPayload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    const QByteArray blob = VaultCrypto::encrypt(newPayload);
    if (blob.isEmpty())
        return;
    if (!OSPaths::writeFileAtomic(path, blob)) {
        qWarning() << "Failed to save default permission state";
        return;
    }
    emit changed();
}

void PermissionsBridge::savePermissionsTest(const QJsonObject &obj)
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QLatin1Char('/') + QStringLiteral("permissions.json");
    QDir().mkpath(QFileInfo(path).absolutePath());
    const QByteArray plain = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    const QByteArray blob = VaultCrypto::encrypt(plain);
    if (blob.isEmpty())
        return;
    if (!OSPaths::writeFileAtomic(path, blob)) {
        qWarning() << "Failed to save permissions test data";
        return;
    }
}

#include "PermissionsBridge.moc"