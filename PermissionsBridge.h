#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

class PermissionsBridge : public QObject
{
    Q_OBJECT

public:
    // Permission types matching QWebEnginePage::Feature and content settings
    enum PermissionType {
        Notifications = 0,
        Geolocation = 1,
        MediaAudioCapture = 2,
        MediaVideoCapture = 3,
        MediaAudioVideoCapture = 4,
        MouseLock = 5,
        DesktopVideoCapture = 6,
        DesktopAudioVideoCapture = 7,
        ClipboardReadWrite = 8,
        LocalFontsAccess = 9,

        // Content settings (per-site)
        Cookies = 10,
        JavaScript = 11,
        Images = 12,
        Popups = 13,
        AutoPlay = 14,
        MixedContent = 15,
        HttpsUpgrade = 16,
        MidiSysex = 17,
        UsbDevices = 18,
        HidDevices = 19,
        SerialPorts = 20,
        WindowPlacement = 21,
        PaymentHandler = 22,
        IdleDetection = 23,
        BluetoothDevices = 24,
        FileSystemAccess = 25,
        StorageAccess = 26,
        TopLevelStorageAccess = 27,

        MaxPermissionType = 27
    };

    // Permission state
    enum PermissionState {
        Default = 0,    // Use global default
        Allow = 1,      // Allow for this site
        Block = 2,      // Block for this site
        Ask = 3         // Ask each time (session only)
    };

    explicit PermissionsBridge(QObject *parent = nullptr);

    // Get all site permissions grouped by origin
    Q_INVOKABLE QJsonArray getPermissions();

    // Get permissions for a specific origin
    Q_INVOKABLE QJsonObject getPermissionsForOrigin(const QString &origin);

    // Set permission for an origin and type
    Q_INVOKABLE void setPermission(const QString &origin, int type, int state);

    // Remove a specific permission (revert to default)
    Q_INVOKABLE void removePermission(const QString &origin, int type);

    // Clear all permissions
    Q_INVOKABLE void clearPermissions();

    // Get available permission types for UI
    Q_INVOKABLE QJsonArray getPermissionTypes();

    // Content settings categories
    Q_INVOKABLE QJsonArray getContentSettingCategories();

    // Get default permission state for a type
    Q_INVOKABLE int getDefaultState(int type);

    // Set default permission state for a type
    Q_INVOKABLE void setDefaultState(int type, int state);

    // Test helper: write permissions directly (bypassing encryption for test setup)
    Q_INVOKABLE void savePermissionsTest(const QJsonObject &obj);

signals:
    void changed();

private:
    QString keyFor(const QString &origin, int type) const;
    QString defaultKeyFor(int type) const;
    QString typeName(int type) const;
    QString categoryName(int type) const;
    int categoryForType(int type) const;
    bool isContentSetting(int type) const;
};

Q_DECLARE_METATYPE(PermissionsBridge::PermissionType)
Q_DECLARE_METATYPE(PermissionsBridge::PermissionState)